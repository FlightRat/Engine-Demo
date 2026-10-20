#include "EditorOperations.h"
#include "StateRules.h"
#include "MaterialRules.h"
#include "OperationHistory.h"
#include "PhysicsEditing.h"
#include "EntitySnapshot.h"
#include "AssetImport.h"
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
#include <set>
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
    OperationHistory history;
    std::map<std::string, std::weak_ptr<SceneObject>> logicalScenes;
    std::map<std::string, EntityEntry> logicalEntities;
    std::map<std::string, std::string> lastEntityIds, lastSceneIds;

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
                entry.AddMember("material_index", static_cast<unsigned>(material.Size()), a);
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
        if (auto* p = reg.try_get<PhysicsComponent>(e)) { components.PushBack("physics", a); out.AddMember("physics", PhysicsJson(p->GetAttr(), a), a); }
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
        for (const auto type : {ENGINE_UTIL::AssetType::MODEL, ENGINE_UTIL::AssetType::TEXTURE, ENGINE_UTIL::AssetType::MUSIC, ENGINE_UTIL::AssetType::SOUNDFX, ENGINE_UTIL::AssetType::SHADER})
            for (const auto& key : assets.GetAssetKeyName(type, true)) resources.PushBack(Text(std::to_string(int(type)) + ":" + key, a), a);
        out.AddMember("assets", std::move(resources), a);
        return out;
    }
    std::string Stamp() { return Stringify(State()); }
    void Observe() {
        MainThread();
        SynchronizeSelection(scenes.GetCurrentScene());
        std::erase_if(entityIds, [](const auto& item) {
            auto s = item.second.scene.lock(); return !s || !s->GetRegistry().GetRegistry().valid(item.second.entity);
        });
        auto now = Stamp();
        if (!baseline.empty() && baseline != now) { ++revision; history.Clear(); }
        baseline = std::move(now);
    }
    Json Status() {
        Json d(rapidjson::kObjectType); auto& a = d.GetAllocator();
        d.AddMember("connected", true, a); d.AddMember("instance", Text(instance, a), a);
        d.AddMember("protocol_version", ProtocolVersion, a);
        d.AddMember("current_scene_id", Text(SceneId(scenes.GetCurrentScene()), a), a);
        auto current = scenes.GetCurrentScene(); d.AddMember("playing", current && current->CheckPlay(), a);
        d.AddMember("write_enabled", !recovery, a);
        d.AddMember("undo_operation_id", Text(history.Top() ? history.Top()->id : "", a), a);
        d.AddMember("redo_operation_id", Text(history.Top(true) ? history.Top(true)->id : "", a), a);
        d.AddMember("undo_count", uint64_t(history.Undo().size()), a); d.AddMember("redo_count", uint64_t(history.Redo().size()), a);
        return d;
    }
    Json HistoryQuery(const Request& r) {
        Json out(rapidjson::kObjectType); auto& a = out.GetAllocator();
        const bool redo = Str(r.params, "stack", "undo") == "redo";
        const auto& stack = redo ? history.Redo() : history.Undo();
        size_t offset = 0, page = r.params.HasMember("page_size") ? r.params["page_size"].GetUint() : 50;
        if (r.params.HasMember("cursor")) {
            auto c = Parse(Str(r.params, "cursor"));
            if (Str(c, "session") != session || Str(c, "stack") != (redo ? "redo" : "undo") ||
                !c.HasMember("revision") || !c["revision"].IsUint64() || c["revision"].GetUint64() != revision ||
                !c.HasMember("serial") || !c["serial"].IsUint64() || c["serial"].GetUint64() != history.Serial() ||
                !c.HasMember("offset") || !c["offset"].IsUint64()) throw Error("revision_conflict", "Stale history cursor");
            offset = size_t(c["offset"].GetUint64());
        }
        if (offset > stack.size()) throw Error("invalid_arguments", "History offset outside snapshot");
        const auto end = (std::min)(offset + page, stack.size()); Value rows(rapidjson::kArrayType);
        for (size_t i = offset; i < end; ++i) {
            const auto& record = stack[stack.size() - i - 1]; Value row(rapidjson::kObjectType);
            row.AddMember("operation_id", Text(record.id, a), a);
            row.AddMember("scope", Text(record.assets ? "assets" : record.createsScene ? "scene-lifecycle" : "scene", a), a);
            auto found = logicalScenes.find(record.logicalScene);
            auto scene = found == logicalScenes.end() ? nullptr : found->second.lock();
            row.AddMember("scene_id", scene ? Text(SceneId(scene), a) : Value(rapidjson::kNullType), a);
            row.AddMember("estimated_bytes", uint64_t(record.Bytes()), a); rows.PushBack(std::move(row), a);
        }
        out.AddMember("entries", std::move(rows), a); out.AddMember("total", uint64_t(stack.size()), a);
        out.AddMember("undo_count", uint64_t(history.Undo().size()), a); out.AddMember("redo_count", uint64_t(history.Redo().size()), a);
        out.AddMember("undo_operation_id", Text(history.Top() ? history.Top()->id : "", a), a);
        out.AddMember("redo_operation_id", Text(history.Top(true) ? history.Top(true)->id : "", a), a);
        out.AddMember("entry_limit", uint64_t(history.EntryLimit()), a); out.AddMember("byte_limit", uint64_t(history.ByteLimit()), a);
        out.AddMember("estimated_bytes", uint64_t(history.Bytes()), a); out.AddMember("history_serial", history.Serial(), a);
        Value next;
        if (end < stack.size()) {
            Json c(rapidjson::kObjectType); auto& ca = c.GetAllocator();
            c.AddMember("session", Text(session, ca), ca); c.AddMember("stack", Text(redo ? "redo" : "undo", ca), ca);
            c.AddMember("revision", revision, ca); c.AddMember("serial", history.Serial(), ca); c.AddMember("offset", uint64_t(end), ca);
            next = Text(Stringify(c), a);
        }
        out.AddMember("next_cursor", std::move(next), a); return out;
    }
    Json Query(const Request& r) {
        if (r.method == "hello" || r.method == "get_editor_status") return Status();
        if (r.method == "get_selection") return Selection();
        if (r.method == "get_history") return HistoryQuery(r);
        if (r.method == "capture_viewport") { ValidateCapture(r); throw Error("capture_unavailable", "No scene renderer is attached", true); }
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
            for (auto [type, label] : {std::pair{ENGINE_UTIL::AssetType::MODEL, "model"}, {ENGINE_UTIL::AssetType::TEXTURE, "texture"}, {ENGINE_UTIL::AssetType::MUSIC, "music"}, {ENGINE_UTIL::AssetType::SOUNDFX, "soundfx"}, {ENGINE_UTIL::AssetType::SHADER, "shader"}}) {
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
    Json Selection() {
        SynchronizeSelection(scenes.GetCurrentScene());
        Json out(rapidjson::kObjectType); auto& a = out.GetAllocator();
        auto s = scenes.GetCurrentScene(); const auto selected = TOOL_CTX().selectedEntity;
        out.AddMember("scene_id", Text(SceneId(s), a), a);
        out.AddMember("selection_serial", SelectionResetSerial, a);
        out.AddMember("entity_id", selected ? Text(EntityId(s, selected->GetEntity()), a) : Value(rapidjson::kNullType), a);
        const auto* identity = selected ? s->GetRegistry().GetRegistry().try_get<Identification>(selected->GetEntity()) : nullptr;
        out.AddMember("name", identity ? Text(identity->name, a) : Value(rapidjson::kNullType), a);
        return out;
    }
    void ValidateCapture(const Request& r) {
        if (Str(r.params, "session_id") != session) throw Error("stale_session", "Refresh editor session");
        auto s = scenes.GetCurrentScene();
        if (!s) throw Error("capture_unavailable", "There is no active edit scene", true);
        if (SceneId(s) != Str(r.params, "scene_id")) throw Error("stale_target", "Capture must target the current scene");
        for (auto& name : scenes.GetAllSceneNames()) if (scenes.GetScene(name)->CheckPlay()) throw Error("play_mode", "Stop play before capture");
        if (r.params["expected_revision"].GetUint64() != revision) throw Error("revision_conflict", "Refresh revision before capture");
    }
    struct VirtualEntity { entt::entity entity{entt::null}; bool mesh = false, deleted = false; std::optional<PhysicsAttributes> physics; std::optional<TransformComponent> transform; std::optional<LightComponent> light; std::optional<MeshRender> render; };
    struct Action { std::string method, key, name, model; VirtualEntity value; std::optional<EntityValues> snapshot; size_t inputIndex = 0; };
    std::vector<entt::entity> DeletionTargets(const std::shared_ptr<SceneObject>& scene, entt::entity root, bool cascade) {
        auto& reg = scene->GetRegistry().GetRegistry(); std::vector<entt::entity> result;
        std::set<entt::entity> visiting, visited;
        std::function<void(entt::entity)> visit = [&](entt::entity e) {
            if (!visiting.insert(e).second) throw Error("unsupported_target", "Hierarchy cycle prevents reversible deletion");
            result.push_back(e);
            for (auto child : Entities(scene)) if (auto* id = reg.try_get<Identification>(child); id && id->parent_id == static_cast<int32_t>(e)) {
                if (!cascade) throw Error("entity_has_children", "Pass cascade=true to delete a subtree");
                if (visited.contains(child)) throw Error("unsupported_target", "Overlapping hierarchy references");
                visit(child);
            }
            visiting.erase(e); visited.insert(e);
        };
        visit(root); return result;
    }
    EntityValues Snapshot(const std::shared_ptr<SceneObject>& scene, entt::entity e) {
        auto v = CaptureEntity(scene->GetRegistry().GetRegistry(), e);
        if (v.identity && v.identity->parent_id != -1) {
            const auto parent = static_cast<entt::entity>(v.identity->parent_id);
            if (!scene->GetRegistry().GetRegistry().valid(parent)) throw Error("unsupported_target", "Entity has a stale parent reference");
            v.logicalParent = EntityKey(scene, parent);
        }
        return v;
    }
    std::vector<Action> Plan(const std::shared_ptr<SceneObject>& s, const Request& r) {
        auto& reg = s->GetRegistry().GetRegistry();
        std::vector<Action> plan;
        std::map<std::string, VirtualEntity> virtuals;
        std::map<std::string, std::string> refs;
        size_t inputIndex = 0;
        auto stage = [&](const std::string& method, const Value& p) {
            Action step; step.method = method; step.inputIndex = inputIndex;
            if (method == "create_object") {
                step.key = Id("staged-"); step.name = Str(p, "name"); step.model = Str(p, "model");
                if (EmptyName(step.name)) throw Error("invalid_arguments", "Object name cannot be blank");
                if (Str(p, "kind") == "mesh") {
                    if (!assets.CheckHasAsset(step.model, ENGINE_UTIL::AssetType::MODEL)) throw Error("asset_not_found", "Model is not loaded: " + step.model);
                    step.value.mesh = true; step.value.transform.emplace();
                    step.value.render.emplace(); step.value.render->ResetMaterial(assets.GetModel(step.model)->GetMeshes());
                } else { step.value.light.emplace(); step.value.light->render = true; }
                virtuals.emplace(step.key, step.value);
                if (p.HasMember("client_ref")) refs.emplace(Str(p, "client_ref"), step.key);
            } else {
                step.key = p.HasMember("target_ref") ? refs.at(Str(p, "target_ref")) : Str(p, "entity_id");
                if (!virtuals.contains(step.key)) {
                    VirtualEntity v; v.entity = Entity(s, step.key); if (auto* p = reg.try_get<PhysicsComponent>(v.entity)) v.physics = p->GetAttr();
                    if (auto* t = reg.try_get<TransformComponent>(v.entity)) v.transform = *t;
                    if (auto* l = reg.try_get<LightComponent>(v.entity)) v.light = *l;
                    if (auto* m = reg.try_get<MeshRender>(v.entity)) v.render = *m;
                    virtuals.emplace(step.key, v);
                }
                auto& value = virtuals.at(step.key);
                if (value.deleted) throw Error("invalid_arguments", "A deleted target cannot be edited or deleted again in the batch");
                if (method == "delete_entity") {
                    const bool cascade = p.HasMember("cascade") && p["cascade"].GetBool();
                    if (value.entity == entt::null) {
                        value.deleted = true; step.value = value; plan.push_back(std::move(step)); return;
                    }
                    auto targets = DeletionTargets(s, value.entity, cascade);
                    for (auto e : targets) {
                        const auto key = EntityId(s, e);
                        if (virtuals.contains(key) && virtuals.at(key).deleted) throw Error("invalid_arguments", "Overlapping deletion in batch");
                        Action deletion; deletion.method = method; deletion.key = key; deletion.inputIndex = inputIndex;
                        deletion.snapshot = Snapshot(s, e);
                        if (virtuals.contains(key)) {
                            const auto& latest = virtuals.at(key);
                            deletion.snapshot->transform = latest.transform; deletion.snapshot->light = latest.light;
                            deletion.snapshot->render = latest.render; deletion.snapshot->physics = latest.physics;
                        }
                        virtuals[key].deleted = true;
                        plan.push_back(std::move(deletion));
                    }
                    return;
                } else if (method == "set_transform") {
                    if (!value.transform) throw Error("unsupported_target", "Target needs Transform");
                    if (value.physics) {
                        ValidatePhysics(*value.physics);
                        if (p.HasMember("scale")) throw Error("unsupported_target", "Use set_physics to change collider size; physics scale edits are unsupported");
                        if (value.entity != entt::null) if (auto* id = reg.try_get<Identification>(value.entity); id && id->parent_id != -1) throw Error("unsupported_target", "Physics pose editing requires a root entity");
                    }
                    UpdateTransform(*value.transform, p);
                } else if (method == "set_material") {
                    if (!value.render) throw Error("unsupported_target", "Target has no MeshRender component");
                    if (!p["material_index"].IsUint64()) throw Error("invalid_arguments", "Material index must fit an unsigned integer");
                    const auto index = p["material_index"].GetUint64();
                    if (index >= value.render->materials.size()) throw Error("invalid_arguments", "Material index is out of range");
                    value.render->materials[index] = UpdatedMaterial(value.render->materials[index], p, assets);
                } else if (method == "add_physics" || method == "set_physics" || method == "remove_physics") {
                    if (method == "add_physics" && value.physics) throw Error("component_exists", "Target already has Physics");
                    if (method != "add_physics" && !value.physics) throw Error("component_not_found", "Target has no Physics");
                    if (method != "remove_physics") {
                        if (!value.transform) throw Error("unsupported_target", "Physics requires a Transform");
                        if (value.entity != entt::null) if (auto* id = reg.try_get<Identification>(value.entity); id && id->parent_id != -1) throw Error("unsupported_target", "Physics editing requires a root entity");
                        value.physics = UpdatedPhysics(value.physics.value_or(PhysicsAttributes{}), p);
                    } else value.physics.reset();
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
                catch (const Error& e) { auto error = e; error.actionIndex = index; throw error; }
                ++index; inputIndex = index;
            }
        } else stage(r.method, r.params);
        return plan;
    }
    std::string SceneKey(const std::shared_ptr<SceneObject>& s) {
        if (!s) return {};
        for (auto& [key, value] : logicalScenes) if (value.lock() == s) return key;
        auto key = SceneId(s); logicalScenes[key] = s; lastSceneIds[key] = key; return key;
    }
    std::shared_ptr<SceneObject> LogicalScene(const std::string& key) {
        if (key.empty()) return {};
        auto it = logicalScenes.find(key);
        auto s = it == logicalScenes.end() ? nullptr : it->second.lock();
        if (!s || !scenes.CheckHasScene(s->GetName()) || scenes.GetScene(s->GetName()) != s)
            throw Error("history_conflict", "History scene no longer exists");
        return s;
    }
    std::string EntityKey(const std::shared_ptr<SceneObject>& s, entt::entity e) {
        for (auto& [key, value] : logicalEntities) if (value.scene.lock() == s && value.entity == e) return key;
        auto key = EntityId(s, e); logicalEntities[key] = {s, e}; lastEntityIds[key] = key; return key;
    }
    entt::entity LogicalEntity(const std::shared_ptr<SceneObject>& s, const std::string& key) {
        auto it = logicalEntities.find(key);
        if (it == logicalEntities.end() || it->second.scene.lock() != s ||
            !s->GetRegistry().GetRegistry().valid(it->second.entity)) throw Error("history_conflict", "History entity no longer exists");
        return it->second.entity;
    }
    void DestroyEntity(const std::shared_ptr<SceneObject>& s, entt::entity e) {
        std::erase_if(entityIds, [&](const auto& pair) { return pair.second.scene.lock() == s && pair.second.entity == e; });
        for (auto& [key, value] : logicalEntities) if (value.scene.lock() == s && value.entity == e) value.entity = entt::null;
        s->GetRegistry().GetRegistry().destroy(e);
    }
    EntityValues Values(const VirtualEntity& v) {
        EntityValues out; out.transform = v.transform; out.light = v.light; out.render = v.render; out.physics = v.physics; return out;
    }
    EntityValues CurrentValues(entt::registry& reg, entt::entity e) {
        EntityValues v;
        if (auto* x = reg.try_get<TransformComponent>(e)) v.transform = *x;
        if (auto* x = reg.try_get<LightComponent>(e)) v.light = *x;
        if (auto* x = reg.try_get<MeshRender>(e)) v.render = *x;
        if (auto* x = reg.try_get<PhysicsComponent>(e)) v.physics = x->GetAttr();
        return v;
    }
    void ApplyValues(entt::registry& reg, entt::entity e, const EntityValues& v) {
        if (v.identity) {
            auto id = *v.identity; id.entity_id = static_cast<int32_t>(e); id.selected = false;
            reg.emplace_or_replace<Identification>(e, std::move(id));
        }
        if (v.transform) reg.emplace_or_replace<TransformComponent>(e, *v.transform);
        if (v.light) reg.emplace_or_replace<LightComponent>(e, *v.light);
        if (v.render) reg.emplace_or_replace<MeshRender>(e, *v.render);
        if (v.mesh) reg.emplace_or_replace<MeshFilter>(e, *v.mesh);
        if (v.cubeCollider) reg.emplace_or_replace<CubeColliderComponent>(e, *v.cubeCollider);
        if (v.sphereCollider) reg.emplace_or_replace<SphereColliderComponent>(e, *v.sphereCollider);
        if (v.physicsTouched) {
            if (v.physics) ApplyPhysics(reg, e, *v.physics); else reg.remove<PhysicsComponent>(e);
        }
        if (v.transform && !v.physicsTouched) SyncPhysicsPose(reg, e);
    }
    entt::entity Recreate(const std::shared_ptr<SceneObject>& s, const EntityValues& v) {
        auto& reg = s->GetRegistry().GetRegistry(); auto e = reg.create();
        try { ApplyValues(reg, e, v); } catch (...) { reg.destroy(e); throw; }
        return e;
    }
    void ValidateImport(const Request& r) {
        contract.Validate("import_asset", r.params);
        if (Str(r.params, "session_id") != session) throw Error("stale_session", "Refresh editor session before import");
        if (recovery) throw Error("recovery_required", "Restart editor after recovery failure");
        if (r.params["expected_revision"].GetUint64() != revision) throw Error("revision_conflict", "Editor changed during import preparation");
        for (const auto& name : scenes.GetAllSceneNames()) if (scenes.GetScene(name)->CheckPlay()) throw Error("play_mode", "Stop play before importing");
        if (assets.FindPayload(ImportType(Str(r.params, "asset_type")), Str(r.params, "asset_key"))) throw Error("asset_name_conflict", "Asset key already exists");
    }
    void ValidateAssetWithdrawal(const ImportBundle& bundle) {
        using ENGINE_UTIL::AssetType;
        std::set<std::pair<AssetType, std::string>> removing;
        for (const auto& entry : bundle.entries) removing.emplace(entry.type, entry.key);
        for (const auto& entry : bundle.entries) {
            auto current = assets.FindPayload(entry.type, entry.key);
            if (!current || *current != entry.payload) throw Error("history_conflict", "Imported resource registration changed");
            if (std::visit([](const auto& p) { return p.use_count(); }, entry.payload) > 3) throw Error("asset_in_use", "Resource is retained by an editor context: " + entry.key);
            if (entry.type == AssetType::MUSIC && ENGINE_SOUNDS::Music::ActiveMusic == std::get<std::shared_ptr<ENGINE_SOUNDS::Music>>(entry.payload)->GetMusicPtr() && (Mix_PlayingMusic() || Mix_PausedMusic())) throw Error("asset_in_use", "Imported music is playing");
            if (entry.type == AssetType::SOUNDFX) {
                auto* chunk = std::get<std::shared_ptr<ENGINE_SOUNDS::SoundFx>>(entry.payload)->GetSoundFxPtr();
                for (int channel = 0; channel < Mix_AllocateChannels(-1); ++channel) if (Mix_GetChunk(channel) == chunk && (Mix_Playing(channel) || Mix_Paused(channel))) throw Error("asset_in_use", "Imported sound effect is playing");
            }
        }
        for (const auto& name : scenes.GetAllSceneNames()) {
            auto& reg = scenes.GetScene(name)->GetRegistry().GetRegistry();
            for (auto e : reg.view<MeshFilter>()) if (removing.contains({AssetType::MODEL, reg.get<MeshFilter>(e).mesh})) throw Error("asset_in_use", "Imported model has a scene consumer");
            for (auto e : reg.view<MeshRender>()) for (const auto& material : reg.get<MeshRender>(e).materials)
                for (const auto& [slot, key] : material.m_textures) if (removing.contains({AssetType::TEXTURE, key})) throw Error("asset_in_use", "Imported texture has a material consumer");
        }
        for (const auto& key : assets.GetAssetKeyName(AssetType::MODEL, true)) {
            if (removing.contains({AssetType::MODEL, key})) continue;
            for (const auto& mesh : assets.GetModel(key)->GetMeshes()) for (const auto& [slot, texture] : mesh.GetDefaultTextures())
                if (removing.contains({AssetType::TEXTURE, texture})) throw Error("asset_in_use", "Imported texture has a model consumer");
        }
    }
    Json Traverse(const Request& r, bool forward) {
        auto* record = history.Top(forward);
        if (!record || record->id != Str(r.params, "operation_id")) throw Error("history_conflict", "Only the current history stack top can be traversed");
        Json out(rapidjson::kObjectType); auto& a = out.GetAllocator();
        out.AddMember("operation_id", Text(Id("operation-"), a), a);
        out.AddMember(Text(forward ? "redone_operation_id" : "undone_operation_id", a), Text(record->id, a), a);
        Value remap(rapidjson::kObjectType), sceneRemap(rapidjson::kObjectType);
        if (hooks.beforeRollback) {
            try { hooks.beforeRollback(); } catch (...) { recovery = true; history.Clear(); throw Error("recovery_required", "History recovery failed"); }
        }
        if (record->assets) {
            if (forward) {
                for (const auto& entry : record->assets->entries) if (assets.FindPayload(entry.type, entry.key)) throw Error("asset_name_conflict", "History resource key is occupied: " + entry.key);
                for (const auto& [type, key] : record->assets->reused) if (!assets.FindPayload(type, key)) throw Error("asset_not_found", "History dependency is unavailable: " + key);
                assets.Publish(record->assets->entries);
            } else {
                ValidateAssetWithdrawal(*record->assets); assets.Withdraw(record->assets->entries);
            }
        } else if (record->createsScene) {
            if (forward) {
                if (scenes.CheckHasScene(record->sceneName)) throw Error("history_conflict", "Scene name is occupied");
                auto previous = LogicalScene(record->previousScene);
                if (previous != scenes.GetCurrentScene()) throw Error("history_conflict", "Scene creation context changed");
                const auto oldId = lastSceneIds.at(record->logicalScene);
                if (!scenes.AddScene(record->sceneName)) throw Error("execution_failed", "Could not recreate scene");
                auto s = scenes.GetScene(record->sceneName);
                try {
                    const auto nextId = SceneId(s);
                    sceneRemap.AddMember(Text(oldId, a), Text(nextId, a), a);
                    logicalScenes[record->logicalScene] = s; lastSceneIds[record->logicalScene] = nextId;
                    out.AddMember("scene_id", Text(nextId, a), a);
                } catch (...) { scenes.DeleteScene(record->sceneName); throw; }
                scenes.SetCurrentScene(record->sceneName);
            } else {
                auto s = LogicalScene(record->logicalScene);
                if (s != scenes.GetCurrentScene() || !Entities(s).empty()) throw Error("history_conflict", "Created scene is not the current empty scene");
                auto previous = LogicalScene(record->previousScene);
                scenes.SetCurrentScene(previous ? previous->GetName() : "");
                if (!scenes.DeleteScene(record->sceneName)) { scenes.SetCurrentScene(record->sceneName); throw Error("history_conflict", "Could not undo scene creation"); }
            }
            ResetSelection();
        } else {
            if (!r.params.HasMember("scene_id")) throw Error("invalid_arguments", "Scene history requires current scene_id");
            auto s = LogicalScene(record->logicalScene);
            if (s != scenes.GetCurrentScene()) throw Error("history_conflict", "History target must be the current scene");
            auto& reg = s->GetRegistry().GetRegistry();
            // Check every dependency before touching ECS.
            std::set<std::string> recreated;
            for (const auto& c : record->changes) {
                const auto& v = forward ? c.after : c.before;
                if ((forward && c.kind == EntityAction::Create) || (!forward && c.kind == EntityAction::Delete)) recreated.insert(c.logicalEntity);
                else if (!recreated.contains(c.logicalEntity) && !( !forward && std::any_of(record->changes.begin(), record->changes.end(), [&](const auto& x) { return x.kind == EntityAction::Delete && x.logicalEntity == c.logicalEntity; }))) LogicalEntity(s, c.logicalEntity);
                if (v.mesh && !assets.CheckHasAsset(v.mesh->mesh, ENGINE_UTIL::AssetType::MODEL)) throw Error("asset_not_found", "History model is unavailable");
                if (v.render) for (const auto& m : v.render->materials) ValidateMaterial(m, assets);
            }
            const auto previousEntityIds = lastEntityIds;
            const auto previousBindings = logicalEntities;
            std::vector<size_t> applied; applied.reserve(record->changes.size());
            std::vector<entt::entity> deferred; deferred.reserve(record->changes.size());
            try {
                for (size_t step = 0; step < record->changes.size(); ++step) {
                    const size_t i = forward ? step : record->changes.size() - step - 1;
                    auto& c = record->changes[i];
                    if (hooks.beforeApply) hooks.beforeApply(step);
                    if (c.kind == EntityAction::Create || c.kind == EntityAction::Delete) {
                        if ((c.kind == EntityAction::Create) == forward) {
                            auto e = Recreate(s, forward ? c.after : c.before);
                            applied.push_back(i);
                            logicalEntities[c.logicalEntity] = {s, e};
                            const auto newId = EntityId(s, e);
                            remap.AddMember(Text(lastEntityIds.at(c.logicalEntity), a), Text(newId, a), a);
                            lastEntityIds[c.logicalEntity] = newId;
                        } else deferred.push_back(LogicalEntity(s, c.logicalEntity));
                    } else {
                        applied.push_back(i);
                        ApplyValues(reg, LogicalEntity(s, c.logicalEntity), forward ? c.after : c.before);
                    }
                }
                for (const auto& c : record->changes) {
                    if ((c.kind == EntityAction::Create && forward) || (c.kind == EntityAction::Delete && !forward)) {
                        const auto& v = forward ? c.after : c.before;
                        if (v.identity) {
                            auto& id = reg.get<Identification>(LogicalEntity(s, c.logicalEntity));
                            id.parent_id = v.logicalParent.empty() ? -1 : static_cast<int32_t>(LogicalEntity(s, v.logicalParent));
                        }
                    }
                }
                out.AddMember("entity_id_remap", std::move(remap), a);
                if (Stringify(out).size() > MaxMessageBytes - 4096) throw Error("operation_too_large", "History mapping exceeds message budget");
            } catch (...) {
                try {
                    for (auto it = applied.rbegin(); it != applied.rend(); ++it) {
                        auto& c = record->changes[*it]; auto e = LogicalEntity(s, c.logicalEntity);
                        if (c.kind == EntityAction::Create || c.kind == EntityAction::Delete) DestroyEntity(s, e);
                        else ApplyValues(reg, e, forward ? c.before : c.after);
                    }
                    lastEntityIds = previousEntityIds; logicalEntities = previousBindings;
                    baseline = Stamp();
                } catch (...) { recovery = true; history.Clear(); throw Error("recovery_required", "History rollback failed"); }
                throw Error("execution_failed", "History traversal failed and was rolled back");
            }
            for (auto e : deferred) DestroyEntity(s, e);
            SynchronizeSelection(s);
        }
        out.AddMember("scene_id_remap", std::move(sceneRemap), a);
        history.Traversed(forward);
        return out;
    }
    Json Write(const Request& r) {
        if (Str(r.params, "session_id") != session) throw Error("stale_session", "Refresh editor session before writing");
        if (recovery) throw Error("recovery_required", "Restart editor after recovery failure");
        for (auto& name : scenes.GetAllSceneNames()) if (scenes.GetScene(name)->CheckPlay()) throw Error("play_mode", "Stop play before editing");
        if (r.params["expected_revision"].GetUint64() != revision) throw Error("revision_conflict", "Read the current editor revision before writing");
        auto s = scenes.GetCurrentScene();
        if (r.method == "import_asset") throw Error("import_unavailable", "Import requires the asynchronous coordinator");
        const bool traversal = r.method == "undo_operation" || r.method == "redo_operation";
        if (r.method != "create_scene" && !traversal && (!s || SceneId(s) != Str(r.params, "scene_id"))) throw Error("stale_target", "Write target must be the current scene");
        if (traversal) {
            if (r.params.HasMember("scene_id") && (!s || SceneId(s) != Str(r.params, "scene_id"))) throw Error("stale_target", "History scene precondition changed");
            return Traverse(r, r.method == "redo_operation");
        }
        Json result(rapidjson::kObjectType); auto& a = result.GetAllocator();
        if (r.method == "set_selection") {
            SelectEntity(s, r.params["entity_id"].IsNull() ? entt::null : Entity(s, Str(r.params, "entity_id")));
            result = Selection();
            result.AddMember("operation_id", Text(Id("operation-"), result.GetAllocator()), result.GetAllocator());
            return result;
        }
        OperationRecord record; record.id = Id("operation-");
        result.AddMember("operation_id", Text(record.id, a), a);
        if (r.method == "create_scene") {
            const auto name = Str(r.params, "name");
            if (EmptyName(name)) throw Error("invalid_arguments", "Scene name cannot be blank");
            if (scenes.CheckHasScene(name)) throw Error("scene_name_conflict", "Scene name already exists");
            record.createsScene = true; record.sceneName = name; record.previousScene = SceneKey(s);
            record.logicalScene = Id("logical-scene-"); history.Admit(record);
            if (!scenes.AddScene(name)) throw Error("execution_failed", "Could not create scene");
            auto created = scenes.GetScene(name);
            try {
                const auto id = SceneId(created); logicalScenes[record.logicalScene] = created; lastSceneIds[record.logicalScene] = id;
                result.AddMember("scene_id", Text(id, a), a);
                history.Commit(std::move(record));
            } catch (...) { scenes.DeleteScene(name); throw; }
            scenes.SetCurrentScene(name); ResetSelection();
        } else {
            auto plan = Plan(s, r); record.logicalScene = SceneKey(s);
            auto& reg = s->GetRegistry().GetRegistry();
            std::map<std::string, EntityValues> values;
            std::map<std::string, std::string> keys;
            record.changes.reserve(plan.size());
            for (const auto& step : plan) {
                EntityChange c;
                if (step.method == "create_object") {
                    c.kind = EntityAction::Create; c.logicalEntity = Id("logical-entity-");
                    c.after = Values(step.value);
                    c.after.identity = Identification{}; c.after.identity->name = step.name;
                    if (step.value.mesh) { c.after.mesh = MeshFilter{}; c.after.mesh->mesh = step.model; }
                    keys[step.key] = c.logicalEntity;
                } else {
                    c.kind = step.method == "set_transform" ? EntityAction::Transform : step.method == "set_material" ? EntityAction::Material : step.method == "set_light" ? EntityAction::Light : step.method == "delete_entity" ? EntityAction::Delete : EntityAction::Physics;
                    if (!keys.contains(step.key)) {
                        const auto e = Entity(s, step.key); keys[step.key] = EntityKey(s, e); values[step.key] = CurrentValues(reg, e);
                    }
                    c.logicalEntity = keys.at(step.key);
                    if (c.kind == EntityAction::Delete) {
                        if (step.snapshot) c.before = *step.snapshot;
                        else { c.before = values.at(step.key); c.before.physicsTouched = true; }
                    }
                    if (c.kind == EntityAction::Transform) { c.before.transform = values[step.key].transform; c.after.transform = step.value.transform; }
                    if (c.kind == EntityAction::Light) { c.before.light = values[step.key].light; c.after.light = step.value.light; }
                    if (c.kind == EntityAction::Material) { c.before.render = values[step.key].render; c.after.render = step.value.render; }
                    if (c.kind == EntityAction::Physics) { c.before.physicsTouched = c.after.physicsTouched = true; c.before.physics = values[step.key].physics; c.after.physics = step.value.physics; }
                }
                if (c.kind == EntityAction::Create) values[step.key] = c.after;
                else if (c.kind != EntityAction::Delete) {
                    auto next = Values(step.value);
                    values[step.key].transform = next.transform; values[step.key].light = next.light;
                    values[step.key].render = next.render; values[step.key].physics = next.physics;
                }
                record.changes.push_back(std::move(c));
            }
            history.Admit(record);
            Value outputs(rapidjson::kArrayType), references(rapidjson::kObjectType);
            outputs.Reserve(static_cast<rapidjson::SizeType>(plan.size()), a);
            size_t applied = 0, actionIndex = 0;
            std::vector<entt::entity> deleted; deleted.reserve(plan.size());
            std::map<std::string, std::string> referenceKeys;
            try {
                for (size_t i = 0; i < plan.size(); ++i) {
                    actionIndex = plan[i].inputIndex;
                    if (hooks.beforeApply) hooks.beforeApply(actionIndex);
                    auto& c = record.changes[i]; entt::entity e;
                    if (c.kind == EntityAction::Create) {
                        e = Recreate(s, c.after); logicalEntities[c.logicalEntity] = {s, e};
                        ++applied;
                        const auto id = EntityId(s, e); lastEntityIds[c.logicalEntity] = id;
                    } else {
                        e = LogicalEntity(s, c.logicalEntity); ++applied;
                        if (c.kind == EntityAction::Delete) deleted.push_back(e); else ApplyValues(reg, e, c.after);
                    }
                    if (c.kind == EntityAction::Delete) {
                        Value row(rapidjson::kObjectType); row.AddMember("entity_id", Text(EntityId(s, e), a), a); row.AddMember("deleted", true, a); outputs.PushBack(std::move(row), a);
                    } else outputs.PushBack(EntityJson(s, e, a), a);
                    if (r.method == "apply_operations") {
                        const auto& p = r.params["operations"][static_cast<rapidjson::SizeType>(actionIndex)]["params"];
                        if (p.HasMember("client_ref")) { references.AddMember(Text(Str(p, "client_ref"), a), Text(EntityId(s, e), a), a); referenceKeys[Str(p, "client_ref")] = c.logicalEntity; }
                    }
                }
                for (const auto& [ref, key] : referenceKeys) {
                    auto e = LogicalEntity(s, key);
                    if (std::find(deleted.begin(), deleted.end(), e) != deleted.end()) references.RemoveMember(ref.c_str());
                }
                if (r.method == "delete_entity") result.AddMember("deleted_entities", Copy(outputs, a), a);
                if (r.method != "apply_operations") result.AddMember("entity", Copy(outputs[0], a), a);
                else { result.AddMember("results", std::move(outputs), a); result.AddMember("references", std::move(references), a); }
                result.AddMember("scene_id", Text(SceneId(s), a), a);
                if (Stringify(result).size() > MaxMessageBytes - 4096) throw Error("operation_too_large", "Operation result exceeds the bridge message budget");
                history.Commit(std::move(record));
                for (auto e : deleted) DestroyEntity(s, e);
                SynchronizeSelection(s);
            } catch (...) {
                const auto failed = actionIndex;
                try {
                    if (hooks.beforeRollback) hooks.beforeRollback();
                    while (applied) {
                        auto& c = record.changes[--applied]; auto e = LogicalEntity(s, c.logicalEntity);
                        if (c.kind == EntityAction::Create) DestroyEntity(s, e); else if (c.kind != EntityAction::Delete) ApplyValues(reg, e, c.before);
                    }
                    baseline = Stamp();
                } catch (...) { recovery = true; history.Clear(); throw Error("recovery_required", "Rollback failed; inspect and restart editor"); }
                throw Error("execution_failed", "Action failed and was rolled back", false, failed);
            }
        }
        return result;
    }

};

EditorOperations::EditorOperations(SceneManager& s, ENGINE_CORE::RESOURCES::AssetManager& a, Contract& c, std::string i, OperationHooks h)
    : m_Impl(std::make_unique<Impl>(s, a, c, std::move(i), std::move(h))) { m_Impl->Observe(); }
EditorOperations::~EditorOperations() = default;
void EditorOperations::ObserveExternalChanges() { m_Impl->Observe(); }
const std::string& EditorOperations::Session() const { return m_Impl->session; }
uint64_t EditorOperations::Revision() const { return m_Impl->revision; }
Json EditorOperations::CheckCapture(const Request& r) {
    auto& impl = *m_Impl; impl.MainThread(); impl.Observe();
    Json out(rapidjson::kObjectType); auto& a = out.GetAllocator();
    try {
        impl.contract.Validate("capture_viewport", r.params); impl.ValidateCapture(r);
        out.AddMember("status", "succeeded", a); out.AddMember("request_id", "", a);
        Value data(rapidjson::kObjectType); data.AddMember("scene_id", Text(Str(r.params, "scene_id"), a), a);
        out.AddMember("data", std::move(data), a);
    } catch (const Error& e) { out = Failure("", e); }
    out.AddMember("session_id", Text(impl.session, out.GetAllocator()), out.GetAllocator());
    out.AddMember("editor_revision", impl.revision, out.GetAllocator()); return out;
}
ENGINE_CORE::RESOURCES::AssetManager& EditorOperations::Assets() { return m_Impl->assets; }
Json EditorOperations::CheckImport(const Request& r) {
    auto& impl = *m_Impl; impl.MainThread(); impl.Observe();
    Json out(rapidjson::kObjectType);
    try { impl.ValidateImport(r); out.AddMember("status", "succeeded", out.GetAllocator()); }
    catch (const Error& e) { out = Failure(r.Id(), e); }
    auto& a = out.GetAllocator(); out.AddMember("session_id", Text(impl.session, a), a); out.AddMember("editor_revision", impl.revision, a); return out;
}
Json EditorOperations::CommitImport(const Request& r, std::shared_ptr<ImportBundle> bundle) {
    auto& impl = *m_Impl; impl.MainThread(); impl.Observe(); Json out(rapidjson::kObjectType);
    try {
        impl.ValidateImport(r);
        if (!bundle || bundle->key != Str(r.params, "asset_key") || bundle->type != ImportType(Str(r.params, "asset_type"))) throw Error("invalid_response", "Prepared import does not match its request");
        OperationRecord record; record.id = impl.Id("operation-"); record.assets = bundle; record.retainedAssetBytes = bundle->estimatedBytes;
        impl.history.Admit(record);
        auto& a = out.GetAllocator(); Value data(rapidjson::kObjectType), added(rapidjson::kArrayType), reused(rapidjson::kArrayType);
        data.AddMember("key", Text(bundle->key, a), a); data.AddMember("type", Text(ImportTypeName(bundle->type), a), a);
        for (const auto& entry : bundle->entries) {
            Value row(rapidjson::kObjectType); row.AddMember("key", Text(entry.key, a), a); row.AddMember("type", Text(ImportTypeName(entry.type), a), a); added.PushBack(std::move(row), a);
            if (impl.assets.FindPayload(entry.type, entry.key)) throw Error("asset_name_conflict", "Prepared key is occupied: " + entry.key);
        }
        for (const auto& [type, key] : bundle->reused) {
            if (!impl.assets.FindPayload(type, key)) throw Error("asset_not_found", "Prepared dependency disappeared: " + key);
            Value row(rapidjson::kObjectType); row.AddMember("key", Text(key, a), a); row.AddMember("type", Text(ImportTypeName(type), a), a); reused.PushBack(std::move(row), a);
        }
        data.AddMember("added", std::move(added), a); data.AddMember("reused", std::move(reused), a);
        out.AddMember("status", "succeeded", a); out.AddMember("request_id", Text(r.Id(), a), a);
        out.AddMember("operation_id", Text(record.id, a), a); out.AddMember("data", std::move(data), a);
        out.AddMember("session_id", Text(impl.session, a), a); out.AddMember("editor_revision", impl.revision + 1, a);
        if (Stringify(out).size() > MaxMessageBytes - 4096) throw Error("import_failed", "Import metadata exceeds message budget");
        impl.assets.Publish(bundle->entries); impl.history.Commit(std::move(record));
        ++impl.revision; impl.baseline = impl.Stamp(); return out;
    } catch (const Error& e) { out = Failure(r.Id(), e); }
    catch (const std::exception& e) { out = Failure(r.Id(), Error("import_failed", e.what())); }
    auto& a = out.GetAllocator(); out.AddMember("session_id", Text(impl.session, a), a); out.AddMember("editor_revision", impl.revision, a); return out;
}
Json EditorOperations::Execute(const Request& r) {
    auto& impl = *m_Impl; impl.MainThread(); impl.Observe();
    Json out;
    try {
        if (r.method != "hello") impl.contract.Validate(r.method, r.params);
        auto data = r.IsWrite() ? impl.Write(r) : impl.Query(r);
        if (r.IsWrite() && r.method != "set_selection") {
            ++impl.revision; impl.baseline = impl.Stamp();

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
