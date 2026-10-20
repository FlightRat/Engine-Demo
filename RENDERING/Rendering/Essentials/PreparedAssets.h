#pragma once
#include "Mesh.h"
#include <atomic>
#include <filesystem>
#include <memory>

namespace ENGINE_RENDERING {
using AssetBytes = std::vector<unsigned char>;
inline constexpr size_t MaxPreparedAssetBytes = 128 * 1024 * 1024;
struct AssetSources {
    std::map<std::string, std::shared_ptr<const AssetBytes>> files;
    std::shared_ptr<std::atomic_bool> cancelled = std::make_shared<std::atomic_bool>(false);
    size_t bytes = 0;
    std::shared_ptr<const AssetBytes> Read(const std::string& path);
    void Check() const;
};
struct PreparedMesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::map<std::string, std::string> textures;
    size_t Bytes() const { return vertices.size() * sizeof(Vertex) + indices.size() * sizeof(unsigned int); }
};
struct PreparedModel {
    std::vector<PreparedMesh> meshes;
    std::map<std::string, std::string> textures;
    size_t bytes = 0;
};
struct PreparedImage {
    int width = 0, height = 0;
    std::vector<unsigned char> rgba;
};
std::string CanonicalAssetPath(const std::string& path);
std::string AssetPathKey(const std::string& path);
PreparedModel PrepareModelFile(const std::string& path, AssetSources& sources);
PreparedImage DecodeAssetImage(const AssetBytes& bytes);
}
