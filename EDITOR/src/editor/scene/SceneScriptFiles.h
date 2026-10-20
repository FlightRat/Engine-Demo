#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace ENGINE_EDITOR {
class SceneScriptFiles {
public:
    using Hook = std::function<void(const char*)>;
    struct Creation {
        std::filesystem::path path;
        bool created = false, committed = false;
        ~Creation();
        void Commit() noexcept { committed = true; }
    };
private:
    std::filesystem::path m_Assets;
    Hook m_Hook;
    void Checkpoint(const char* name) const { if (m_Hook) m_Hook(name); }
public:
    void SetAssetRoot(const std::filesystem::path& assets);
    std::filesystem::path AssetRoot() const;
    std::filesystem::path ScriptPath(const std::string& name) const;
    std::unique_ptr<Creation> Prepare(const std::string& name) const;
    void Rename(const std::string& from, const std::string& to) const;
    void SetHook(Hook hook) { m_Hook = std::move(hook); }
    static void ValidateName(const std::string& name);
    static bool SameName(const std::string& a, const std::string& b);
    static std::string Read(const std::filesystem::path& path);
    static std::string SyntaxError(const std::filesystem::path& path);
    static const char* Template();
};
}
