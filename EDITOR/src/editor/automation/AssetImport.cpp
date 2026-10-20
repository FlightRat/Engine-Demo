#include "AssetImport.h"
#include <Rendering/Essentials/TextureLoader.h>
#include <Rendering/Essentials/ShaderLoader.h>
#include <filesystem>
#include <set>
#include <climits>

namespace ENGINE_EDITOR::Automation {
using ENGINE_UTIL::AssetType;
using namespace ENGINE_RENDERING;
using namespace ENGINE_CORE::RESOURCES;
AssetType ImportType(const std::string& name) {
    if (name == "model") return AssetType::MODEL;
    if (name == "texture") return AssetType::TEXTURE;
    if (name == "shader") return AssetType::SHADER;
    if (name == "music") return AssetType::MUSIC;
    if (name == "soundfx") return AssetType::SOUNDFX;
    throw Error("invalid_arguments", "Unsupported asset_type");
}
std::string ImportTypeName(AssetType type) {
    switch (type) {
    case AssetType::MODEL: return "model";
    case AssetType::TEXTURE: return "texture";
    case AssetType::SHADER: return "shader";
    case AssetType::MUSIC: return "music";
    case AssetType::SOUNDFX: return "soundfx";
    default: throw Error("invalid_arguments", "Unsupported asset type");
    }
}
namespace {
std::string LocalPath(const std::string& value) {
    if (value.find('\0') != std::string::npos) throw Error("invalid_arguments", "Source path contains a null character");
    const auto p = std::filesystem::path(reinterpret_cast<const char8_t*>(value.c_str()));
    if (!p.is_absolute() || value.starts_with("\\\\") || value.starts_with("//")) throw Error("invalid_arguments", "Import requires an absolute local file path");
    return CanonicalAssetPath(value);
}
std::string SourceText(const AssetBytes& source) {
    if (std::find(source.begin(), source.end(), 0) != source.end()) throw Error("invalid_arguments", "Shader source contains a null character");
    return {source.begin(), source.end()};
}
}
PreparedImport PrepareImport(const Value& params, const std::shared_ptr<std::atomic_bool>& cancelled) {
    PreparedImport out; out.type = ImportType(Str(params, "asset_type")); out.key = Str(params, "asset_key");
    if (out.key.find_first_not_of(" \t\r\n") == std::string::npos) throw Error("invalid_arguments", "Asset key cannot be blank");
    out.sources.cancelled = cancelled;
    out.pixelArt = params.HasMember("pixel_art") && params["pixel_art"].GetBool();
    if (out.type == AssetType::SHADER) {
        out.path = LocalPath(Str(params, "vertex_path"));
        out.vertex = SourceText(*out.sources.Read(out.path));
        out.fragment = SourceText(*out.sources.Read(LocalPath(Str(params, "fragment_path"))));
        out.estimatedBytes = out.vertex.size() + out.fragment.size();
    } else {
        out.path = LocalPath(Str(params, "path")); out.sources.Read(out.path);
        if (out.type == AssetType::MODEL) {
            out.model = PrepareModelFile(out.path, out.sources);
            std::map<std::string, std::string> rewritten;
            for (const auto& [key, path] : out.model->textures) {
                const auto resourceKey = out.key + "_" + key;
                if (resourceKey.size() > 256) throw Error("invalid_arguments", "Model key leaves no room for dependency keys");
                rewritten[resourceKey] = path;
                auto image = DecodeAssetImage(*out.sources.Read(path));
                out.estimatedBytes += image.rgba.size() * 2 + out.sources.Read(path)->size();
                if (out.estimatedBytes > 256 * 1024 * 1024) throw Error("history_capacity_exceeded", "Model textures exceed the import budget");
                out.images.emplace(resourceKey, std::move(image));
            }
            for (auto& mesh : out.model->meshes) for (auto& [slot, key] : mesh.textures) if (!key.empty()) key = out.key + "_" + key;
            out.model->textures = std::move(rewritten); out.estimatedBytes += out.model->bytes * 2;
        } else if (out.type == AssetType::TEXTURE) {
            auto image = DecodeAssetImage(*out.sources.Read(out.path)); out.estimatedBytes += image.rgba.size() * 2 + out.sources.Read(out.path)->size();
            out.images.emplace(out.key, std::move(image));
        }
    }
    if (out.type == AssetType::MUSIC) out.estimatedBytes += out.sources.bytes * 2;
    if (out.type == AssetType::SOUNDFX) out.estimatedBytes += out.sources.bytes;
    if (out.estimatedBytes > 256 * 1024 * 1024) throw Error("history_capacity_exceeded", "Prepared asset exceeds the retained history budget");
    out.sources.Check(); return out;
}
ImportUpload::ImportUpload(PreparedImport source, AssetManager& manager)
    : prepared(std::move(source)), assets(manager), bundle(std::make_shared<ImportBundle>()) {
    bundle->key = prepared.key; bundle->type = prepared.type; bundle->estimatedBytes = prepared.estimatedBytes;
    bundle->entries.reserve(prepared.images.size() + 1);
    if (prepared.model) meshes.reserve(prepared.model->meshes.size());
}
bool ImportUpload::Tick() {
    if (done) return true;
    prepared.sources.Check();
    if (image < prepared.images.size()) {
        auto it = prepared.images.begin(); std::advance(it, image);
        const auto path = prepared.model ? prepared.model->textures.at(it->first) : prepared.path;
        const auto source = prepared.sources.Read(path);
        if (assets.CheckHasAsset(it->first, AssetType::TEXTURE)) {
            const auto* existing = assets.Provenance(AssetType::TEXTURE, it->first);
            if (!existing || existing->path != path || existing->pixelArt != prepared.pixelArt || !existing->source || *existing->source != *source)
                throw Error("asset_name_conflict", "Texture dependency name conflicts with different content: " + it->first);
            bundle->reused.emplace_back(AssetType::TEXTURE, it->first);
            bundle->estimatedBytes -= source->size() + it->second.rgba.size() * 2;
        } else {
            auto value = TextureLoader::CreatePrepared(it->second, prepared.pixelArt, path);
            LoadedAsset entry; entry.type = AssetType::TEXTURE; entry.key = it->first; entry.payload = std::move(value);
            entry.provenance = {path, prepared.pixelArt, source, {}}; entry.estimatedBytes = source->size() + it->second.rgba.size() * 2;
            bundle->entries.push_back(std::move(entry));
        }
        ++image; return false;
    }
    if (prepared.model) {
        if (!glCreateBuffers || !glGetString(GL_VERSION)) throw Error("import_unavailable", "No active OpenGL context for model upload");
        if (mesh < prepared.model->meshes.size()) {
            auto& data = prepared.model->meshes[mesh];
            meshes.emplace_back(std::move(data.vertices), std::move(data.indices), std::move(data.textures));
            if (glGetError() != GL_NO_ERROR) throw Error("import_failed", "GPU model upload failed");
            ++mesh; return false;
        }
        auto model = std::make_shared<Model>(std::move(meshes));
        const auto directory = std::filesystem::path(reinterpret_cast<const char8_t*>(prepared.path.c_str())).parent_path().generic_u8string();
        model->SetDir(std::string(reinterpret_cast<const char*>(directory.data()), directory.size()));
        LoadedAsset entry; entry.type = AssetType::MODEL; entry.key = prepared.key; entry.payload = std::move(model);
        entry.provenance.path = prepared.path;
        bundle->entries.push_back(std::move(entry));
    } else if (prepared.type == AssetType::SHADER) {
        LoadedAsset entry; entry.type = prepared.type; entry.key = prepared.key;
        entry.payload = ShaderLoader::CompileSources(prepared.vertex, prepared.fragment);
        entry.provenance.path = prepared.path; bundle->entries.push_back(std::move(entry));
    } else if (prepared.type == AssetType::MUSIC || prepared.type == AssetType::SOUNDFX) {
        const auto source = prepared.sources.Read(prepared.path);
        if (source->size() > INT_MAX) throw Error("import_failed", "Audio source is too large");
        int frequency = 0, channels = 0; Uint16 format = 0;
        if (!Mix_QuerySpec(&frequency, &format, &channels)) throw Error("import_unavailable", "Audio backend is not initialized");
        auto* stream = SDL_RWFromConstMem(source->data(), int(source->size()));
        if (!stream) throw Error("import_failed", std::string("Audio stream failed: ") + SDL_GetError());
        LoadedAsset entry; entry.type = prepared.type; entry.key = prepared.key; entry.provenance = {prepared.path, false, source, {}};
        if (prepared.type == AssetType::MUSIC) {
            auto* value = Mix_LoadMUS_RW(stream, 1);
            if (!value) throw Error("import_failed", std::string("Music decode failed: ") + Mix_GetError());
            ENGINE_SOUNDS::SoundParams params{.name = prepared.key, .filename = prepared.path, .duration = Mix_MusicDuration(value)};
            entry.payload = std::make_shared<ENGINE_SOUNDS::Music>(params, MusicPtr{value}, source);
        } else {
            auto* value = Mix_LoadWAV_RW(stream, 1);
            if (!value) throw Error("import_failed", std::string("SoundFX decode failed: ") + Mix_GetError());
            const auto decoded = size_t(value->alen);
            SoundFxPtr guard(value);
            if (decoded > 256 * 1024 * 1024 - bundle->estimatedBytes) throw Error("history_capacity_exceeded", "Decoded audio exceeds history budget");
            bundle->estimatedBytes += decoded;
            const auto rate = size_t(frequency) * channels * (SDL_AUDIO_BITSIZE(format) / 8);
            ENGINE_SOUNDS::SoundParams params{.name = prepared.key, .filename = prepared.path, .duration = rate ? double(decoded) / rate : 0};
            entry.payload = std::make_shared<ENGINE_SOUNDS::SoundFx>(params, std::move(guard));
        }
        bundle->entries.push_back(std::move(entry));
    }
    done = true; return true;
}
}
