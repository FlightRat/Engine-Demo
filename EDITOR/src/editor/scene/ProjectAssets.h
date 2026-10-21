#pragma once
#include "SceneScriptFiles.h"
#include "PlaybackController.h"
#include "../automation/AssetImport.h"
#include <sol/sol.hpp>

namespace ENGINE_EDITOR {
// A separate, declarative startup manifest. Scene scripts remain compile-only in Editing.
inline std::vector<Automation::Json> ReadProjectAssets(const std::filesystem::path& assetRoot) {
    const auto manifest = assetRoot / "scripts/preload.lua";
    if (!std::filesystem::exists(manifest)) return {};
    sol::state lua; // No engine bindings or standard libraries in the manifest environment.
    auto result = lua.safe_script(SceneScriptFiles::Read(manifest), sol::script_pass_on_error, "preload.lua");
    if (!result.valid()) { sol::error error = result; throw error; }
    if (result.return_count() != 1 || !result.get<sol::object>().is<sol::table>())
        throw std::runtime_error("preload.lua must return an asset list");
    sol::table list = result.get<sol::table>();
    std::vector<Automation::Json> requests;
    for (size_t i = 1; i <= list.size(); ++i) {
        sol::table entry = list[i];
        Automation::Json request; request.SetObject(); auto& allocator = request.GetAllocator();
        for (const char* key : {"asset_type", "asset_key", "path", "vertex_path", "fragment_path"}) {
            sol::object value = entry[key];
            if (!value.valid() || value == sol::lua_nil) continue;
            auto text = value.as<std::string>();
            if (std::string(key).ends_with("path")) {
                auto path = std::filesystem::path(std::u8string(text.begin(), text.end()));
                const auto utf8 = std::filesystem::absolute(assetRoot / path).lexically_normal().u8string();
                text.assign(utf8.begin(), utf8.end());
            }
            Automation::Set(request, key, Automation::Text(text, allocator), allocator);
        }
        const auto kind = Automation::ImportType(Automation::Str(request, "asset_type"));
        if (Automation::Str(request, "asset_key").empty()) throw std::runtime_error("preload.lua asset_key is required");
        for (const char* key : kind == ENGINE_UTIL::AssetType::SHADER
            ? std::vector<const char*>{"vertex_path", "fragment_path"} : std::vector<const char*>{"path"})
            if (Automation::Str(request, key).empty()) throw std::runtime_error(std::string("preload.lua requires ") + key);
        Automation::Set(request, "pixel_art", Automation::Value(entry.get_or("pixel_art", false)), allocator);
        requests.push_back(std::move(request));
    }
    return requests;
}
inline void LoadProjectAssets(const std::filesystem::path& assetRoot, ENGINE_CORE::RESOURCES::AssetManager& assets) {
    if (!PlaybackController::Get().CanEdit()) throw std::runtime_error("Project assets must be preloaded while Editing");
    for (const auto& request : ReadProjectAssets(assetRoot)) {
        const auto key = Automation::Str(request, "asset_key");
        try {
            const auto type = Automation::ImportType(Automation::Str(request, "asset_type"));
            if (assets.CheckHasAsset(key, type)) continue;
            Automation::ImportUpload upload(Automation::PrepareImport(request, std::make_shared<std::atomic_bool>(false)), assets);
            while (!upload.Tick()) {}
            assets.Publish(upload.Result()->entries);
        } catch (const std::exception& error) {
            throw std::runtime_error("Project asset [" + key + "]: " + error.what());
        }
    }
}
}
