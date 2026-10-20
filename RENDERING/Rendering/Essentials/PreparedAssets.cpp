#include "PreparedAssets.h"
#include "ModelCommon.h"
#include "TextureCommon.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/ProgressHandler.hpp>
#include <climits>
#include <cctype>
#include <assimp/IOSystem.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/postprocess.h>
#include <stb_image.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>

namespace ENGINE_RENDERING {
std::string CanonicalAssetPath(const std::string& path) {
    auto p = std::filesystem::path(reinterpret_cast<const char8_t*>(path.c_str()));
    auto value = std::filesystem::weakly_canonical(std::filesystem::absolute(p)).generic_u8string();
    std::string out(reinterpret_cast<const char*>(value.data()), value.size());
#ifdef _WIN32
    // Path identity is case-insensitive on the supported Windows editor.
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return char(std::tolower(c)); });
#endif
    return out;
}
std::string AssetPathKey(const std::string& path) {
    uint64_t hash = 14695981039346656037ull;
    for (unsigned char c : path) { hash ^= c; hash *= 1099511628211ull; }
    std::ostringstream out; out << std::hex << hash; return out.str();
}
void AssetSources::Check() const {
    if (cancelled->load()) throw std::runtime_error("Asset preparation cancelled");
}
std::shared_ptr<const AssetBytes> AssetSources::Read(const std::string& path) {
    Check();
    if (path.starts_with("//") || path.starts_with("\\\\")) throw std::runtime_error("Network asset dependencies are unsupported");
    const auto key = CanonicalAssetPath(path);
    if (auto it = files.find(key); it != files.end()) return it->second;
    if (files.size() >= 4096) throw std::runtime_error("Too many asset source dependencies");
    const auto p = std::filesystem::path(reinterpret_cast<const char8_t*>(key.c_str()));
    if (!std::filesystem::is_regular_file(p)) throw std::runtime_error("Asset source is not a readable file: " + key);
    std::ifstream input(p, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Cannot open asset source: " + key);
    const auto length = input.tellg();
    if (length <= 0 || uint64_t(length) > MaxPreparedAssetBytes - bytes) throw std::runtime_error("Asset source exceeds the preparation budget: " + key);
    auto data = std::make_shared<AssetBytes>(size_t(length)); input.seekg(0);
    // Bounded reads give cancellation a chance even for slow filesystem sources.
    for (size_t offset = 0; offset < data->size();) {
        Check(); const auto count = (std::min)(size_t(1024 * 1024), data->size() - offset);
        if (!input.read(reinterpret_cast<char*>(data->data() + offset), count)) throw std::runtime_error("Cannot read asset source: " + key);
        offset += count;
    }
    bytes += data->size(); files.emplace(key, data); return data;
}
namespace {
class CancelProgress final : public Assimp::ProgressHandler {
    std::shared_ptr<std::atomic_bool> cancelled;
public:
    explicit CancelProgress(std::shared_ptr<std::atomic_bool> value) : cancelled(std::move(value)) {}
    bool Update(float) override { return !cancelled->load(); }
};
class MemoryStream final : public Assimp::IOStream {
    std::shared_ptr<const AssetBytes> bytes;
    size_t offset = 0;
public:
    explicit MemoryStream(std::shared_ptr<const AssetBytes> source) : bytes(std::move(source)) {}
    size_t Read(void* destination, size_t size, size_t count) override {
        if (!size) return 0;
        const auto items = (std::min)(count, (bytes->size() - offset) / size);
        std::memcpy(destination, bytes->data() + offset, items * size); offset += items * size; return items;
    }
    size_t Write(const void*, size_t, size_t) override { return 0; }
    aiReturn Seek(size_t amount, aiOrigin origin) override {
        size_t next = 0;
        if (origin == aiOrigin_SET) next = amount;
        else if (origin == aiOrigin_CUR) { if (amount > bytes->size() - offset) return aiReturn_FAILURE; next = offset + amount; }
        else { if (amount > bytes->size()) return aiReturn_FAILURE; next = bytes->size() - amount; }
        if (next > bytes->size()) return aiReturn_FAILURE; offset = next; return aiReturn_SUCCESS;
    }
    size_t Tell() const override { return offset; }
    size_t FileSize() const override { return bytes->size(); }
    void Flush() override {}
};
class SnapshotIO final : public Assimp::IOSystem {
    AssetSources& sources;
public:
    std::string error;
    explicit SnapshotIO(AssetSources& value) : sources(value) {}
    bool Exists(const char* path) const override {
        try { return std::filesystem::is_regular_file(std::filesystem::path(reinterpret_cast<const char8_t*>(path))); } catch (...) { return false; }
    }
    char getOsSeparator() const override { return '/'; }
    Assimp::IOStream* Open(const char* path, const char* mode) override {
        if (!mode || mode[0] != 'r') return nullptr;
        try { return new MemoryStream(sources.Read(path)); } catch (const std::exception& e) { error = e.what(); return nullptr; }
    }
    void Close(Assimp::IOStream* stream) override { delete stream; }
};
glm::vec3 Vector(const aiVector3D& v) {
    if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) throw std::runtime_error("Model contains a non-finite vertex value");
    return {v.x, v.y, v.z};
}
}
PreparedModel PrepareModelFile(const std::string& path, AssetSources& sources) {
    if (!ModelRegistry::IsSupportedPath(path)) throw std::runtime_error("Unsupported model format");
    Assimp::Importer importer;
    auto* io = new SnapshotIO(sources); importer.SetIOHandler(io);
    importer.SetProgressHandler(new CancelProgress(sources.cancelled));
    const auto* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        throw std::runtime_error(std::string("Model parsing failed for ") + path + ": " + importer.GetErrorString() + " " + io->error);
    if (!io->error.empty()) throw std::runtime_error("Model dependency read failed: " + io->error);
    if (scene->mNumMeshes > 4096) throw std::runtime_error("Model exceeds the mesh-part budget");
    PreparedModel out;
    const auto directory = std::filesystem::path(reinterpret_cast<const char8_t*>(path.c_str())).parent_path();
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        sources.Check(); const auto* mesh = scene->mMeshes[i];
        const uint64_t required = uint64_t(mesh->mNumVertices) * sizeof(Vertex) + uint64_t(mesh->mNumFaces) * 3 * sizeof(unsigned);
        if (required > MaxPreparedAssetBytes - out.bytes || !mesh->mVertices) throw std::runtime_error("Model geometry exceeds the preparation budget");
        PreparedMesh data; data.vertices.reserve(mesh->mNumVertices); data.indices.reserve(size_t(mesh->mNumFaces) * 3);
        for (unsigned n = 0; n < mesh->mNumVertices; ++n) {
            if ((n & 4095) == 0) sources.Check();
            Vertex vertex{}; vertex.Position = Vector(mesh->mVertices[n]);
            if (mesh->HasNormals()) vertex.Normal = Vector(mesh->mNormals[n]);
            if (mesh->mTextureCoords[0]) { const auto uv = Vector(mesh->mTextureCoords[0][n]); vertex.TexCoords = {uv.x, uv.y}; }
            if (mesh->HasTangentsAndBitangents()) { vertex.Tangent = Vector(mesh->mTangents[n]); vertex.Bitangent = Vector(mesh->mBitangents[n]); }
            data.vertices.push_back(vertex);
        }
        for (unsigned n = 0; n < mesh->mNumFaces; ++n) {
            const auto& face = mesh->mFaces[n];
            if (face.mNumIndices != 3) throw std::runtime_error("Model contains non-triangle geometry");
            for (unsigned k = 0; k < 3; ++k) {
                if (face.mIndices[k] >= mesh->mNumVertices) throw std::runtime_error("Model contains an invalid vertex index");
                data.indices.push_back(face.mIndices[k]);
            }
        }
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            const auto* material = scene->mMaterials[mesh->mMaterialIndex];
            for (const auto& slot : TextureRegistry::GetSlots()) {
                std::string chosen;
                for (unsigned t = 0; t < material->GetTextureCount(slot.assimpType); ++t) {
                    aiString value;
                    if (material->GetTexture(slot.assimpType, t, &value) != AI_SUCCESS) throw std::runtime_error("Invalid model texture reference");
                    std::string relative = value.C_Str();
                    if (relative.empty() || relative[0] == '*' || scene->GetEmbeddedTexture(relative.c_str())) throw std::runtime_error("Embedded model textures cannot be decoded by this import path");
                    std::replace(relative.begin(), relative.end(), '\\', '/');
                    const auto file = directory / std::filesystem::path(reinterpret_cast<const char8_t*>(relative.c_str()));
                    const auto u8 = file.generic_u8string(); const auto source = CanonicalAssetPath(std::string(reinterpret_cast<const char*>(u8.data()), u8.size()));
                    sources.Read(source); const auto key = AssetPathKey(source);
                    if (out.textures.contains(key) && out.textures.at(key) != source) throw std::runtime_error("Model dependency key collision");
                    out.textures[key] = source; chosen = key;
                    if (out.textures.size() > 512) throw std::runtime_error("Model exceeds the texture-dependency budget");
                }
                data.textures[slot.key] = chosen;
            }
        }
        out.bytes += data.Bytes(); out.meshes.push_back(std::move(data));
    }
    if (out.meshes.empty()) throw std::runtime_error("Model contains no renderable meshes");
    return out;
}
PreparedImage DecodeAssetImage(const AssetBytes& bytes) {
    int width = 0, height = 0, channels = 0;
    if (bytes.size() > INT_MAX || !stbi_info_from_memory(bytes.data(), int(bytes.size()), &width, &height, &channels) ||
        width <= 0 || height <= 0 || uint64_t(width) * height * 4 > MaxPreparedAssetBytes) throw std::runtime_error("Image dimensions or content exceed the preparation budget");
    stbi_set_flip_vertically_on_load_thread(0);
    auto* pixels = stbi_load_from_memory(bytes.data(), int(bytes.size()), &width, &height, &channels, 4);
    if (!pixels) throw std::runtime_error(std::string("Image decode failed: ") + stbi_failure_reason());
    PreparedImage out; out.width = width; out.height = height;
    try { out.rgba.assign(pixels, pixels + size_t(width) * height * 4); } catch (...) { stbi_image_free(pixels); throw; }
    stbi_image_free(pixels); return out;
}
}
