#include "EditorOperations.h"
#include "StateRules.h"
#include "../scene/SceneManager.h"
#include "../scene/SceneObject.h"
#include <Core/Resources/AssetManager.h>
#include <Core/ECS/Components/Identification.h>
#include <Core/ECS/Components/MeshFilter.h>
#include <Core/ECS/Components/MeshRender.h>
#include <Core/ECS/Components/PhysicsComponent.h>
#include <Core/ECS/Components/ScriptComponent.h>
#include <Utilities/EngineUtilities.h>
#include <array>
#include <map>
#include <optional>
#include <random>
#include <thread>
#include <sstream>
#include <iomanip>

namespace ENGINE_EDITOR::Automation {
using namespace ENGINE_CORE::ECS;
namespace {
glm::vec3 Vec(const Value& v) { return {v[0].GetFloat(), v[1].GetFloat(), v[2].GetFloat()}; }
Value VecJson(const glm::vec3& v, Allocator& a) {
    Value out(rapidjson::kArrayType); out.PushBack(v.x, a).PushBack(v.y, a).PushBack(v.z, a); return out;
}
Value TransformJson(const TransformComponent& t, Allocator& a) {
    Value out(rapidjson::kObjectType);
    out.AddMember("space", "local", a);
    out.AddMember("position", VecJson(t.position, a), a);
    out.AddMember("rotation_degrees", VecJson(t.rotation_eular, a), a);
    out.AddMember("scale", VecJson(t.scale, a), a);
    return out;
}
Value LightJson(const LightComponent& l, Allocator& a) {
    Value out(rapidjson::kObjectType);
    out.AddMember("type", Text(l.type, a), a);
    out.AddMember("color", VecJson(l.color, a), a);
    out.AddMember("intensity", l.intensity, a);
    out.AddMember("render", l.render, a);
    if (l.type == "direction_light") out.AddMember("direction", VecJson(l.direction, a), a);
    else out.AddMember("position", VecJson(l.pos, a), a);
    if (l.type == "point_light") {
        out.AddMember("constant", l.constant, a); out.AddMember("linear", l.linear, a);
        out.AddMember("quadratic", l.quadratic, a); out.AddMember("range", l.range, a);
    }
    if (l.type == "area_light") {
        out.AddMember("rotation_degrees", VecJson(l.rotation_eular, a), a);
        out.AddMember("half_width", l.half_width, a); out.AddMember("half_height", l.half_height, a);
    }
    return out;
}
void UpdateTransform(TransformComponent& t, const Value& p) {
    if (p.HasMember("position")) t.position = Vec(p["position"]);
    if (p.HasMember("rotation_degrees")) { t.rotation_eular = Vec(p["rotation_degrees"]); SynchronizeRotation(t); }
    if (p.HasMember("scale")) t.scale = Vec(p["scale"]);
}
void UpdateLight(LightComponent& l, const Value& p) {
    const auto type = Str(p, "type", l.type);
    for (auto i = p.MemberBegin(); i != p.MemberEnd(); ++i) {
        const std::string key = i->name.GetString();
        const bool point = key == "constant" || key == "linear" || key == "quadratic" || key == "range";
        const bool area = key == "half_width" || key == "half_height" || key == "rotation_degrees";
        if ((point && type != "point_light") || (area && type != "area_light") ||
            (key == "direction" && type != "direction_light") || (key == "position" && type == "direction_light"))
            throw Error("invalid_arguments", "Field is not applicable to light type: " + key);
    }
    SetLightType(l, type);
    if (p.HasMember("color")) l.color = Vec(p["color"]);
    if (p.HasMember("position")) l.pos = Vec(p["position"]);
    if (p.HasMember("direction")) {
        if (!SetLightDirection(l, Vec(p["direction"]))) throw Error("invalid_arguments", "Light direction must be nonzero");
    }
    if (p.HasMember("rotation_degrees")) { l.rotation_eular = Vec(p["rotation_degrees"]); SynchronizeRotation(l); }
    if (p.HasMember("render")) l.render = p["render"].GetBool();
    for (auto [key, value] : std::array<std::pair<const char*, float*>, 7>{{
        {"intensity", &l.intensity}, {"constant", &l.constant}, {"linear", &l.linear},
        {"quadratic", &l.quadratic}, {"range", &l.range}, {"half_width", &l.half_width}, {"half_height", &l.half_height}}})
        if (p.HasMember(key)) *value = p[key].GetFloat();
}
bool EmptyName(const std::string& name) { return name.find_first_not_of(" \r\n\t") == std::string::npos; }
std::string NewSession() {
    std::random_device random;
    std::ostringstream out; out << std::hex;
    for (int i = 0; i != 4; ++i) out << std::setw(8) << std::setfill('0') << random();
    return out.str();
}
}

struct EditorOperations::Impl {
    SceneManager& scenes;
    ENGINE_CORE::RESOURCES::AssetManager& assets;
    Contract& contract;
    std::string instance, session = NewSession();
    std::thread::id owner = std::this_thread::get_id();
    uint64_t revision = 0, nextId = 0;
    std::string baseline;
    bool recovery = false;
    OperationHooks hooks;
    struct SceneEntry { std::weak_ptr<SceneObject> scene; std::string id; };
    struct EntityEntry { std::weak_ptr<SceneObject> scene; entt::entity entity; };
    std::vector<SceneEntry> sceneIds;
    std::map<std::string, EntityEntry> entityIds;
    struct History { std::string id, after; std::vector<std::function<void()>> inverse; };
    std::optional<History> history;

    Impl(SceneManager& s, ENGINE_CORE::RESOURCES::AssetManager& a, Contract& c, std::string i, OperationHooks h)
        : scenes(s), assets(a), contract(c), instance(std::move(i)), hooks(std::move(h)) {}
    void MainThread() const {
        if (std::this_thread::get_id() != owner) throw std::logic_error("Editor operations require the main thread");
    }
    std::string Id(const char* prefix) { return std::string(prefix) + std::to_string(++nextId); }
    std::string SceneId(const std::shared_ptr<SceneObject>& scene) {
        if (!scene) return {};
        std::erase_if(sceneIds, [](const SceneEntry& x) { return x.scene.expired(); });
        for (auto& x : sceneIds) if (x.scene.lock() == scene) return x.id;
        auto id = Id("scene-"); sceneIds.push_back({scene, id}); return id;
    }
    std::shared_ptr<SceneObject> Scene(const std::string& id) {
        for (const auto& name : scenes.GetAllSceneNames()) {
            auto s = scenes.GetScene(name); if (SceneId(s) == id) return s;
        }
        throw Error("stale_target", "Scene no longer exists");
    }
    std::string EntityId(const std::shared_ptr<SceneObject>& s, entt::entity e) {
        for (auto& [id, x] : entityIds) if (x.scene.lock() == s && x.entity == e) return id;
        auto id = Id("entity-"); entityIds.emplace(id, EntityEntry{s, e}); return id;
    }
    entt::entity Entity(const std::shared_ptr<SceneObject>& s, const std::string& id) {
        auto it = entityIds.find(id);
        if (it == entityIds.end() || it->second.scene.lock() != s || !s->GetRegistry().GetRegistry().valid(it->second.entity))
            throw Error("stale_target", "Entity no longer exists in this scene");
        return it->second.entity;
    }
    Value EntityJson(const std::shared_ptr<SceneObject>& s, entt::entity e, Allocator& a) {
        auto& reg = s->GetRegistry().GetRegistry();
        Value out(rapidjson::kObjectType), components(rapidjson::kArrayType);
        out.AddMember("entity_id", Text(EntityId(s, e), a), a);
        if (auto* id = reg.try_get<Identification>(e)) {
            out.AddMember("name", Text(id->name, a), a); out.AddMember("group", Text(id->group, a), a);
            out.AddMember("parent", id->parent_id, a);
        }
        if (auto* t = reg.try_get<TransformComponent>(e)) { out.AddMember("transform", TransformJson(*t, a), a); components.PushBack("transform", a); }
        if (auto* l = reg.try_get<LightComponent>(e)) { out.AddMember("light", LightJson(*l, a), a); components.PushBack("light", a); }
        if (auto* m = reg.try_get<MeshFilter>(e)) { out.AddMember("model", Text(m->mesh, a), a); components.PushBack("mesh", a); }
        if (auto* m = reg.try_get<MeshRender>(e)) {
            Value material(rapidjson::kArrayType);
            for (const auto& mat : m->materials) {
                Value entry(rapidjson::kObjectType);
                entry.AddMember("shading_model", Text(mat.shadingModel, a), a);
                entry.AddMember("color", VecJson(glm::vec3(mat.color), a), a); entry.AddMember("alpha", mat.color.a, a);
                entry.AddMember("metallic", mat.metallic, a); entry.AddMember("roughness", mat.roughness, a);
                entry.AddMember("ao", mat.ao, a); entry.AddMember("use_texture", mat.m_useTexture, a);
                Value textures(rapidjson::kObjectType);
                for (const auto& [key, value] : mat.m_textures) textures.AddMember(Text(key, a), Text(value, a), a);
                entry.AddMember("textures", std::move(textures), a); material.PushBack(std::move(entry), a);
            }
            out.AddMember("materials", std::move(material), a); out.AddMember("visible", m->shouldRender, a); out.AddMember("flip_uv", m->flipUV, a);
        }
        if (reg.all_of<PhysicsComponent>(e)) components.PushBack("physics", a);
        if (reg.all_of<ScriptComponent>(e)) components.PushBack("script", a);
        // Unknown component additions/removals also invalidate undo, without exposing raw pointers.
        size_t count = 0; for (auto&& [id, storage] : reg.storage()) if (storage.contains(e)) ++count;
        out.AddMember("component_count", static_cast<unsigned>(count), a);
        out.AddMember("components", std::move(components), a);
        return out;
    }
    std::vector<entt::entity> Entities(const std::shared_ptr<SceneObject>& s) {
        std::vector<entt::entity> out;
        for (auto e : s->GetRegistry().GetRegistry().view<entt::entity>()) out.push_back(e);
        std::sort(out.begin(), out.end(), [](auto a, auto b) { return entt::to_integral(a) < entt::to_integral(b); });
        return out;
    }
    Json State() {
        Json out(rapidjson::kObjectType); auto& a = out.GetAllocator();
        out.AddMember("manual_serial", ManualEditSerial, a);
        out.AddMember("active", Text(scenes.GetCurrentSceneName(), a), a);
        Value list(rapidjson::kArrayType);
        for (auto& name : scenes.GetAllSceneNames()) {
            auto s = scenes.GetScene(name); Value entry(rapidjson::kObjectType), ents(rapidjson::kArrayType);
            entry.AddMember("scene_id", Text(SceneId(s), a), a); entry.AddMember("name", Text(name, a), a);
            entry.AddMember("playing", s->CheckPlay(), a);
            for (auto e : Entities(s)) ents.PushBack(EntityJson(s, e, a), a);
            entry.AddMember("entities", std::move(ents), a); list.PushBack(std::move(entry), a);
        }
        out.AddMember("scenes", std::move(list), a);
        Value resources(rapidjson::kArrayType);
        for (const auto type : {ENGINE_UTIL::AssetType::MODEL, ENGINE_UTIL::AssetType::TEXTURE})
            for (const auto& key : assets.GetAssetKeyName(type, true)) resources.PushBack(Text(std::to_string(int(type)) + ":" + key, a), a);
        out.AddMember("assets", std::move(resources), a);
        return out;
    }
    std::string Stamp() { return Stringify(State()); }
    void Observe() {
        MainThread();
        std::erase_if(entityIds, [](const auto& item) {
            auto s = item.second.scene.lock(); return !s || !s->GetRegistry().GetRegistry().valid(item.second.entity);
        });
        auto now = Stamp();
        if (!baseline.empty() && baseline != now) { ++revision; history.reset(); }
        baseline = std::move(now);
    }
    Json Status() {
        Json d(rapidjson::kObjectType); auto& a = d.GetAllocator();
        d.AddMember("connected", true, a); d.AddMember("instance", Text(instance, a), a);
        d.AddMember("protocol_version", ProtocolVersion, a);
        d.AddMember("current_scene_id", Text(SceneId(scenes.GetCurrentScene()), a), a);
        auto current = scenes.GetCurrentScene(); d.AddMember("playing", current && current->CheckPlay(), a);
        d.AddMember("write_enabled", !recovery, a);
        d.AddMember("undo_operation_id", Text(history ? history->id : "", a), a);
        return d;
    }
    Json Query(const Request& r) {
        if (r.method == "hello" || r.method == "get_editor_status") return Status();
        auto state = State(); auto& a = state.GetAllocator();
        const auto sceneId = Str(r.params, "scene_id", SceneId(scenes.GetCurrentScene()));
        Value rows(rapidjson::kArrayType);
        std::string signature = r.method + ":" + sceneId + ":" + Str(r.params, "name_filter") + ":" + Str(r.params, "component_filter") + ":" + Str(r.params, "asset_type");
        if (r.method == "get_scene_state") {
            if (!sceneId.empty()) {
                auto s = Scene(sceneId);
                for (auto e : Entities(s)) {
                    auto row = EntityJson(s, e, a);
                    if (Str(row, "name").find(Str(r.params, "name_filter")) == std::string::npos) continue;
                    auto filter = Str(r.params, "component_filter");
                    bool matches = filter.empty();
                    for (const auto& c : row["components"].GetArray()) if (filter == c.GetString()) matches = true;
                    if (matches) rows.PushBack(std::move(row), a);
                }
            }
            for (auto& s : state["scenes"].GetArray()) s.RemoveMember("entities");
            state.RemoveMember("assets"); state.RemoveMember("manual_serial");
            state.AddMember("scene_id", Text(sceneId, a), a);
        } else if (r.method == "list_assets") {
            const auto filter = Str(r.params, "asset_type");
            for (auto [type, label] : {std::pair{ENGINE_UTIL::AssetType::MODEL, "model"}, {ENGINE_UTIL::AssetType::TEXTURE, "texture"}}) {
                if (!filter.empty() && filter != label) continue;
                for (const auto& key : assets.GetAssetKeyName(type, true)) {
                    Value row(rapidjson::kObjectType); row.AddMember("key", Text(key, a), a); row.AddMember("type", Text(label, a), a);
                    rows.PushBack(std::move(row), a);
                }
            }
            state.SetObject();
        } else throw Error("unknown_method", "Unknown query");
        size_t offset = 0, page = r.params.HasMember("page_size") ? r.params["page_size"].GetUint() : 50;
        const auto cursor = Str(r.params, "cursor");
        if (!cursor.empty()) {
            auto token = Parse(cursor);
            if (!token.IsObject() || Str(token, "session") != session || Str(token, "query") != signature ||
                !token.HasMember("revision") || !token["revision"].IsUint64() || token["revision"].GetUint64() != revision ||
                !token.HasMember("offset") || !token["offset"].IsUint64()) throw Error("revision_conflict", "Stale or mismatched page cursor");
            offset = size_t(token["offset"].GetUint64());
        }
        if (offset > rows.Size()) throw Error("invalid_arguments", "Page offset is outside snapshot");
        Value pageRows(rapidjson::kArrayType);
        const size_t end = (std::min)(offset + page, size_t(rows.Size()));
        for (size_t i = offset; i < end; ++i) pageRows.PushBack(Copy(rows[rapidjson::SizeType(i)], a), a);
        state.AddMember(Text(r.method == "list_assets" ? "assets" : "entities", a), std::move(pageRows), a);
        state.AddMember("total", rows.Size(), a);
        Value next;
        if (end < rows.Size()) {
            Json token(rapidjson::kObjectType); auto& ta = token.GetAllocator();
            token.AddMember("session", Text(session, ta), ta); token.AddMember("query", Text(signature, ta), ta);
            token.AddMember("revision", revision, ta); token.AddMember("offset", uint64_t(end), ta);
            next = Text(Stringify(token), a);
        }
        state.AddMember("next_cursor", std::move(next), a);
        return state;
    }
    struct VirtualEntity { entt::entity entity{entt::null}; bool mesh = false, physics = false; std::optional<TransformComponent> transform; std::optional<LightComponent> light; };
    struct Action { std::string method, key, name, model; VirtualEntity value; };
    std::vector<Action> Plan(const std::shared_ptr<SceneObject>& s, const Request& r) {
        auto& reg = s->GetRegistry().GetRegistry();
        std::vector<Action> plan;
        std::map<std::string, VirtualEntity> virtuals;
        std::map<std::string, std::string> refs;
        auto stage = [&](const std::string& method, const Value& p) {
            Action step; step.method = method;
            if (method == "create_object") {
                step.key = Id("staged-"); step.name = Str(p, "name"); step.model = Str(p, "model");
                if (EmptyName(step.name)) throw Error("invalid_arguments", "Object name cannot be blank");
                if (Str(p, "kind") == "mesh") {
                    if (!assets.CheckHasAsset(step.model, ENGINE_UTIL::AssetType::MODEL)) throw Error("asset_not_found", "Model is not loaded: " + step.model);
                    step.value.mesh = true; step.value.transform.emplace();
                } else { step.value.light.emplace(); step.value.light->render = true; }
                virtuals.emplace(step.key, step.value);
                if (p.HasMember("client_ref")) refs.emplace(Str(p, "client_ref"), step.key);
            } else {
                step.key = p.HasMember("target_ref") ? refs.at(Str(p, "target_ref")) : Str(p, "entity_id");
                if (!virtuals.contains(step.key)) {
                    VirtualEntity v; v.entity = Entity(s, step.key); v.physics = reg.all_of<PhysicsComponent>(v.entity);
                    if (auto* t = reg.try_get<TransformComponent>(v.entity)) v.transform = *t;
                    if (auto* l = reg.try_get<LightComponent>(v.entity)) v.light = *l;
                    virtuals.emplace(step.key, v);
                }
                auto& value = virtuals.at(step.key);
                if (method == "set_transform") {
                    if (value.physics || !value.transform) throw Error("unsupported_target", "Target needs Transform and must not have physics");
                    UpdateTransform(*value.transform, p);
                } else {
                    if (!value.light) throw Error("unsupported_target", "Target has no Light component");
                    UpdateLight(*value.light, p);
                }
                step.value = value;
            }
            plan.push_back(std::move(step));
        };
        if (r.method == "apply_operations") {
            size_t index = 0;
            for (const auto& action : r.params["operations"].GetArray()) {
                try { stage(Str(action, "method"), action["params"]); }
                catch (const Error& e) { throw Error(e.code, "Action " + std::to_string(index) + ": " + e.what(), e.retryable, index); }
                ++index;
            }
        } else stage(r.method, r.params);
        return plan;
    }
    void Reverse(std::vector<std::function<void()>>& inverse) {
        if (hooks.beforeRollback) hooks.beforeRollback();
        for (auto it = inverse.rbegin(); it != inverse.rend(); ++it) (*it)();
        ResetSelection();
    }
    Json Write(const Request& r) {
        if (Str(r.params, "session_id") != session) throw Error("stale_session", "Refresh editor session before writing");
        if (recovery) throw Error("recovery_required", "Restart editor after recovery failure");
        for (auto& name : scenes.GetAllSceneNames()) if (scenes.GetScene(name)->CheckPlay()) throw Error("play_mode", "Stop play before editing");
        if (r.params["expected_revision"].GetUint64() != revision) throw Error("revision_conflict", "Read the current editor revision before writing");
        auto s = scenes.GetCurrentScene();
        if (r.method != "create_scene" && (!s || SceneId(s) != Str(r.params, "scene_id"))) throw Error("stale_target", "Write target must be the current scene");
        Json result(rapidjson::kObjectType); auto& a = result.GetAllocator();
        if (r.method == "undo_operation") {
            if (!history || history->id != Str(r.params, "operation_id") || history->after != Stamp()) throw Error("history_conflict", "Latest undo record is no longer valid");
            auto id = history->id;
            try { Reverse(history->inverse); } catch (...) { recovery = true; history.reset(); throw Error("recovery_required", "Undo recovery failed; inspect and restart editor"); }
            history.reset(); result.AddMember("undone_operation_id", Text(id, a), a);
            result.AddMember("operation_id", Text(Id("operation-"), a), a);
            return result;
        }
        History record{Id("operation-"), {}, {}};
        if (r.method == "create_scene") {
            auto name = Str(r.params, "name");
            if (EmptyName(name)) throw Error("invalid_arguments", "Scene name cannot be blank");
            if (scenes.CheckHasScene(name)) throw Error("scene_name_conflict", "Scene name already exists");
            const auto oldName = scenes.GetCurrentSceneName();
            if (!scenes.AddScene(name)) throw Error("execution_failed", "Could not create scene");
            auto created = scenes.GetScene(name);
            record.inverse.push_back([this, name, oldName, weak = std::weak_ptr<SceneObject>(created)] {
                auto target = weak.lock();
                if (!target || !Entities(target).empty()) throw Error("history_conflict", "Created scene is no longer empty");
                scenes.SetCurrentScene(oldName);
                if (!scenes.DeleteScene(name)) throw Error("history_conflict", "Could not remove created scene");
            });
            scenes.SetCurrentScene(name); ResetSelection();
            result.AddMember("scene_id", Text(SceneId(created), a), a);
        } else {
            auto plan = Plan(s, r);
            auto& reg = s->GetRegistry().GetRegistry();
            std::map<std::string, entt::entity> created;
            Value outputs(rapidjson::kArrayType), references(rapidjson::kObjectType);
            record.inverse.reserve(plan.size());
            size_t index = 0;
            try {
                for (const auto& step : plan) {
                    if (hooks.beforeApply) hooks.beforeApply(index);
                    entt::entity e;
                    if (step.method == "create_object") {
                        ENGINE_CORE::ECS::Entity object{s->GetRegistry(), step.name, ""}; e = object.GetEntity();
                        record.inverse.push_back([s, e] { auto& r = s->GetRegistry().GetRegistry(); if (r.valid(e)) r.destroy(e); });
                        if (step.value.mesh) {
                            object.AddComponent<TransformComponent>();
                            auto& filter = object.AddComponent<MeshFilter>(); filter.mesh = step.model; filter.changed = false;
                            auto& render = object.AddComponent<MeshRender>(); render.ResetMaterial(assets.GetModel(step.model)->GetMeshes());
                        } else object.AddComponent<LightComponent>(*step.value.light);
                        created.emplace(step.key, e);
                    } else {
                        e = created.contains(step.key) ? created.at(step.key) : Entity(s, step.key);
                        if (step.method == "set_transform") {
                            auto old = reg.get<TransformComponent>(e);
                            record.inverse.push_back([s, e, old] { s->GetRegistry().GetRegistry().replace<TransformComponent>(e, old); });
                            reg.replace<TransformComponent>(e, *step.value.transform);
                        } else {
                            auto old = reg.get<LightComponent>(e);
                            record.inverse.push_back([s, e, old] { s->GetRegistry().GetRegistry().replace<LightComponent>(e, old); });
                            reg.replace<LightComponent>(e, *step.value.light);
                        }
                    }
                    outputs.PushBack(EntityJson(s, e, a), a);
                    if (r.method == "apply_operations") {
                        const auto& p = r.params["operations"][rapidjson::SizeType(index)]["params"];
                        if (p.HasMember("client_ref")) references.AddMember(Text(Str(p, "client_ref"), a), Text(EntityId(s, e), a), a);
                    }
                    ++index;
                }
            } catch (...) {
                try { Reverse(record.inverse); baseline = Stamp(); }
                catch (...) { recovery = true; history.reset(); throw Error("recovery_required", "Rollback failed; inspect and restart editor"); }
                throw Error("execution_failed", "Action " + std::to_string(index) + " failed and was rolled back", false, index);
            }
            if (r.method != "apply_operations") result.AddMember("entity", Copy(outputs[0], a), a);
            else { result.AddMember("results", std::move(outputs), a); result.AddMember("references", std::move(references), a); }
            result.AddMember("scene_id", Text(SceneId(s), a), a);
        }
        result.AddMember("operation_id", Text(record.id, a), a);
        history = std::move(record);
        return result;
    }
};

EditorOperations::EditorOperations(SceneManager& s, ENGINE_CORE::RESOURCES::AssetManager& a, Contract& c, std::string i, OperationHooks h)
    : m_Impl(std::make_unique<Impl>(s, a, c, std::move(i), std::move(h))) { m_Impl->Observe(); }
EditorOperations::~EditorOperations() = default;
void EditorOperations::ObserveExternalChanges() { m_Impl->Observe(); }
const std::string& EditorOperations::Session() const { return m_Impl->session; }
uint64_t EditorOperations::Revision() const { return m_Impl->revision; }
Json EditorOperations::Execute(const Request& r) {
    auto& impl = *m_Impl; impl.MainThread(); impl.Observe();
    Json out;
    try {
        if (r.method != "hello") impl.contract.Validate(r.method, r.params);
        auto data = r.IsWrite() ? impl.Write(r) : impl.Query(r);
        if (r.IsWrite()) {
            ++impl.revision; impl.baseline = impl.Stamp();
            if (impl.history) impl.history->after = impl.baseline;
        }
        out.SetObject(); auto& a = out.GetAllocator();
        out.AddMember("status", "succeeded", a); out.AddMember("request_id", Text(r.Id(), a), a);
        if (data.HasMember("operation_id")) out.AddMember("operation_id", Copy(data["operation_id"], a), a);
        // data owns a different memory pool. A Value move would leave dangling
        // strings when its Document allocator is destroyed on return.
        out.AddMember("data", Copy(data, a), a);
    } catch (const Error& e) { out = Failure(r.Id(), e); }
    catch (const std::exception& e) { out = Failure(r.Id(), Error("execution_failed", e.what())); }
    auto& a = out.GetAllocator(); out.AddMember("session_id", Text(impl.session, a), a);
    out.AddMember("editor_revision", impl.revision, a);
    return out;
}
}
