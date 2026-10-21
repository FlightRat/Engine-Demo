#include "SceneScriptFiles.h"
#include <sol/sol.hpp>
#include <fstream>
#include <atomic>
#include <stdexcept>
#include <algorithm>
#include <Windows.h>

namespace ENGINE_EDITOR {
namespace fs = std::filesystem;
namespace {
std::wstring Wide(const std::string& value) {
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (!count) throw std::runtime_error("Scene name is not valid UTF-8");
    std::wstring result(count, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), count);
    return result;
}
fs::path Temporary(const fs::path& directory) {
    static std::atomic<uint64_t> sequence{0};
    return directory / (L".scene-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(++sequence) + L".tmp");
}
void MoveNew(const fs::path& source, const fs::path& target) {
    if (!MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH))
        throw std::system_error(GetLastError(), std::system_category(), "Cannot publish scene script");
}
}
const char* SceneScriptFiles::Template() {
    return "return {\n    on_build = function()\n    end,\n\n    on_start = function()\n    end,\n\n    on_update = function(dt)\n    end,\n\n    on_stop = function()\n    end,\n}\n";
}
std::string SceneScriptFiles::Read(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        const auto utf8 = path.u8string();
        throw std::runtime_error("Cannot read scene script: " + std::string(utf8.begin(), utf8.end()));
    }
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
SceneScriptFiles::Creation::~Creation() {
    if (!created || committed) return;
    // Only remove the template we own. Never remove externally changed bytes.
    try { if (Read(path) == Template()) { std::error_code error; fs::remove(path, error); } } catch (...) {}
}
bool SceneScriptFiles::SameName(const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return a == b;
    const auto wa = Wide(a), wb = Wide(b);
    return CompareStringOrdinal(wa.data(), static_cast<int>(wa.size()), wb.data(), static_cast<int>(wb.size()), TRUE) == CSTR_EQUAL;
}
void SceneScriptFiles::ValidateName(const std::string& name) {
    if (name.empty()) throw std::runtime_error("Scene name cannot be empty");
    const auto wide = Wide(name);
    if (wide.size() + 4 > 255 || name.back() == ' ' || name.back() == '.') throw std::runtime_error("Invalid Scene filename length or suffix");
    for (unsigned char c : name) if (c < 32 || std::string("<>:\"/\\|?*").find(static_cast<char>(c)) != std::string::npos)
        throw std::runtime_error("Scene name must be a Windows filename, not a path");
    auto base = name.substr(0, name.find('.'));
    while (!base.empty() && base.back() == ' ') base.pop_back();
    for (auto& c : base) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (base == "CON" || base == "PRN" || base == "AUX" || base == "NUL" || base == "CLOCK$" ||
        (base.size() == 4 && (base.starts_with("COM") || base.starts_with("LPT")) && base[3] >= '1' && base[3] <= '9'))
        throw std::runtime_error("Scene name is a reserved Windows device name");
    const auto wb = base.empty() ? std::wstring{} : Wide(base);
    if (wb.size() == 4 && (wb.substr(0, 3) == L"COM" || wb.substr(0, 3) == L"LPT") && (wb[3] == L'\u00b9' || wb[3] == L'\u00b2' || wb[3] == L'\u00b3'))
        throw std::runtime_error("Scene name is a reserved Windows device name");
}
void SceneScriptFiles::SetAssetRoot(const fs::path& assets) { m_Assets = fs::absolute(assets).lexically_normal(); }
fs::path SceneScriptFiles::AssetRoot() const {
    if (!m_Assets.empty()) return m_Assets;
    for (auto directory = fs::current_path(); !directory.empty();) {
        if (fs::is_directory(directory / "assets")) return directory / "assets";
        if (fs::is_directory(directory / "EDITOR" / "assets")) return directory / "EDITOR" / "assets";
        auto parent = directory.parent_path(); if (parent == directory) break; directory = parent;
    }
    throw std::runtime_error("Cannot locate EDITOR/assets; configure the asset root");
}
fs::path SceneScriptFiles::ScriptPath(const std::string& name) const {
    ValidateName(name); return AssetRoot() / "scripts" / fs::path(Wide(name + ".lua"));
}
std::unique_ptr<SceneScriptFiles::Creation> SceneScriptFiles::Prepare(const std::string& name) const {
    auto result = std::make_unique<Creation>(); result->path = ScriptPath(name);
    if (fs::exists(result->path)) {
        if (!fs::is_regular_file(result->path)) throw std::runtime_error("Scene script path is not a file");
        Read(result->path); return result;
    }
    fs::create_directories(result->path.parent_path());
    const auto temporary = Temporary(result->path.parent_path());
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::system_error(GetLastError(), std::system_category(), "Cannot create scene script");
    try {
        const std::string bytes = Template(); DWORD written = 0;
        if (!WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) || written != bytes.size() || !FlushFileBuffers(file))
            throw std::runtime_error("Cannot write scene script");
        CloseHandle(file); file = INVALID_HANDLE_VALUE;
        Checkpoint("before_publish"); MoveNew(temporary, result->path); result->created = true;
        Checkpoint("after_publish"); return result;
    } catch (...) {
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        std::error_code error; fs::remove(temporary, error); throw;
    }
}
void SceneScriptFiles::Rename(const std::string& from, const std::string& to) const {
    const auto source = ScriptPath(from), target = ScriptPath(to);
    if (from == to) return;
    if (!fs::is_regular_file(source)) throw std::runtime_error("Source scene script is missing");
    if (fs::exists(target) && !fs::equivalent(source, target)) throw std::runtime_error("Destination scene script already exists");
    const auto temporary = Temporary(source.parent_path());
    Checkpoint("before_rename"); MoveNew(source, temporary);
    try { Checkpoint("during_rename"); MoveNew(temporary, target); }
    catch (...) { MoveNew(temporary, source); throw; }
}
std::string SceneScriptFiles::SyntaxError(const fs::path& path) {
    try {
        sol::state lua; const auto bytes = Read(path); const auto text = path.u8string();
        auto chunk = lua.load(bytes, std::string(text.begin(), text.end()));
        if (!chunk.valid()) { sol::error error = chunk; return error.what(); }
        return {};
    } catch (const std::exception& error) { return error.what(); }
}
}
