#pragma once
#include "Protocol.h"
#include "StateRules.h"
#include <Core/ECS/Components/MeshRender.h>
#include <Core/Resources/AssetManager.h>
#include <Rendering/Essentials/TextureCommon.h>
#include <Utilities/EngineUtilities.h>

namespace ENGINE_EDITOR::Automation {
using ENGINE_CORE::ECS::Material;
inline bool MaterialEqual(const Material& a, const Material& b) {
    return a.shadingModel == b.shadingModel && a.color == b.color && a.metallic == b.metallic &&
        a.roughness == b.roughness && a.ao == b.ao && a.m_useTexture == b.m_useTexture && a.m_textures == b.m_textures;
}
inline void ValidateMaterial(const Material& m, ENGINE_CORE::RESOURCES::AssetManager& assets) {
    for (float v : {m.color.r, m.color.g, m.color.b, m.color.a, m.metallic, m.roughness, m.ao})
        if (!std::isfinite(v) || v < 0 || v > 1) throw Error("invalid_arguments", "Material values must be finite in [0,1]");
    for (const auto& [slot, key] : m.m_textures) {
        const auto& slots = ENGINE_RENDERING::TextureRegistry::GetSlots();
        if (std::none_of(slots.begin(), slots.end(), [&](const auto& s) { return s.key == slot; }))
            throw Error("invalid_arguments", "Unknown texture slot: " + slot);
        if (key.empty()) continue;
        if (!assets.CheckHasAsset(key, ENGINE_UTIL::AssetType::TEXTURE)) throw Error("asset_not_found", "Texture is not loaded: " + key);
        const auto texture = assets.GetTexture(key);
        using T = ENGINE_RENDERING::Texture::TextureType;
        if (!texture || (texture->GetType() != T::PIXEL && texture->GetType() != T::BLENDED))
            throw Error("unsupported_target", "Material texture must be a loaded 2D image: " + key);
    }
}
inline bool CommitMaterial(Material& destination, const Material& next, ENGINE_CORE::RESOURCES::AssetManager& assets, bool manual) {
    if (manual && !PlaybackController::Get().CanEdit()) return false;
    if (MaterialEqual(destination, next)) return false;
    ValidateMaterial(next, assets);
    destination = next;
    if (manual) NotifyManualEdit();
    return true;
}
inline Material UpdatedMaterial(const Material& old, const Value& p, ENGINE_CORE::RESOURCES::AssetManager& assets) {
    auto next = old;
    if (p.HasMember("color")) for (int i = 0; i < 3; ++i) next.color[i] = p["color"][i].GetFloat();
    for (auto [key, value] : {std::pair{"alpha", &next.color.a}, {"metallic", &next.metallic},
                            {"roughness", &next.roughness}, {"ao", &next.ao}})
        if (p.HasMember(key)) *value = p[key].GetFloat();
    if (p.HasMember("use_texture")) next.m_useTexture = p["use_texture"].GetBool();
    if (p.HasMember("textures")) for (auto it = p["textures"].MemberBegin(); it != p["textures"].MemberEnd(); ++it)
        next.m_textures[it->name.GetString()] = it->value.IsNull() ? "" : std::string(it->value.GetString(), it->value.GetStringLength());
    ValidateMaterial(next, assets);
    return next;
}
}
