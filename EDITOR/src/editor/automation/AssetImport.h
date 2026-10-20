#pragma once
#include "Protocol.h"
#include <Core/Resources/AssetManager.h>
#include <Rendering/Essentials/PreparedAssets.h>
#include <Utilities/EngineUtilities.h>

namespace ENGINE_EDITOR::Automation {
struct PreparedImport {
    ENGINE_UTIL::AssetType type{};
    std::string key, path, vertex, fragment;
    bool pixelArt = false;
    ENGINE_RENDERING::AssetSources sources;
    std::optional<ENGINE_RENDERING::PreparedModel> model;
    std::map<std::string, ENGINE_RENDERING::PreparedImage> images;
    size_t estimatedBytes = 0;
};
struct ImportBundle {
    std::string key;
    ENGINE_UTIL::AssetType type{};
    std::vector<ENGINE_CORE::RESOURCES::LoadedAsset> entries;
    std::vector<std::pair<ENGINE_UTIL::AssetType, std::string>> reused;
    size_t estimatedBytes = 0;
};
ENGINE_UTIL::AssetType ImportType(const std::string& name);
std::string ImportTypeName(ENGINE_UTIL::AssetType type);
PreparedImport PrepareImport(const Value& params, const std::shared_ptr<std::atomic_bool>& cancelled);
// Each owner-thread tick uploads one texture or one mesh; registration is separate.
class ImportUpload {
    PreparedImport prepared;
    ENGINE_CORE::RESOURCES::AssetManager& assets;
    std::shared_ptr<ImportBundle> bundle;
    std::vector<ENGINE_RENDERING::Mesh> meshes;
    size_t image = 0, mesh = 0;
    bool done = false;
public:
    ImportUpload(PreparedImport source, ENGINE_CORE::RESOURCES::AssetManager& manager);
    bool Tick();
    std::shared_ptr<ImportBundle> Result() const { return done ? bundle : nullptr; }
    const ImportBundle& PendingBundle() const { return *bundle; }
};
}
