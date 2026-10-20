#include "SceneManager.h"
#include "SceneObject.h"
#include "PlaybackController.h"
#include "../automation/StateRules.h"
#include <Logger/Logger.h>
#include <algorithm>

namespace ENGINE_EDITOR {
SceneManager& SceneManager::GetInstance() { static SceneManager value; return value; }
bool SceneManager::CheckHasScene(const std::string& name) {
    return std::any_of(m_mapScene.begin(), m_mapScene.end(), [&](const auto& item) { return SceneScriptFiles::SameName(item.first, name); });
}
bool SceneManager::AddScene(const std::string& name, std::unique_ptr<SceneScriptFiles::Creation>* receipt) {
    try {
        if (!PlaybackController::Get().CanEdit()) throw std::runtime_error("Stop playback before creating a Scene");
        SceneScriptFiles::ValidateName(name);
        if (CheckHasScene(name)) throw std::runtime_error("Scene name already exists");
        auto scene = std::make_shared<SceneObject>(name);
        auto script = m_Scripts.Prepare(name);
        m_mapScene.emplace(name, std::move(scene));
        if (receipt) *receipt = std::move(script); else script->Commit();
        m_LastError.clear(); return true;
    } catch (const std::exception& error) { m_LastError = error.what(); ENGINE_ERROR("Cannot create Scene [{}]: {}", name, m_LastError); return false; }
}
bool SceneManager::CreateAndActivate(const std::string& name) {
    // Allocate the active name before publication so allocation failure cannot
    // leave a created scene reported as a failed activation.
    std::string active = name;
    if (!AddScene(name)) return false;
    Automation::ResetSelection(); m_sCurrentScene.swap(active);
    auto error = SceneScriptFiles::SyntaxError(m_Scripts.ScriptPath(name));
    if (!error.empty()) ENGINE_ERROR("Scene script: {}", error);
    return true;
}
bool SceneManager::DeleteScene(const std::string& name) {
    if (!PlaybackController::Get().CanEdit()) return false;
    if (SceneScriptFiles::SameName(name, m_sCurrentScene)) return false;
    for (auto i = m_mapScene.begin(); i != m_mapScene.end(); ++i)
        if (SceneScriptFiles::SameName(i->first, name)) { m_mapScene.erase(i); return true; }
    return false;
}
bool SceneManager::ChangeSceneName(const std::string& oldName, const std::string& newName) {
    try {
        if (!PlaybackController::Get().CanEdit()) throw std::runtime_error("Stop playback before renaming a Scene");
        SceneScriptFiles::ValidateName(newName);
        auto found = m_mapScene.find(oldName);
        if (found == m_mapScene.end()) throw std::runtime_error("Scene no longer exists");
        if (oldName == newName) return true;
        if (!SceneScriptFiles::SameName(oldName, newName) && CheckHasScene(newName)) throw std::runtime_error("Scene name already exists");
        auto candidate = m_mapScene;
        auto scene = found->second;
        candidate.erase(oldName); candidate.emplace(newName, scene);
        std::string active = m_sCurrentScene == oldName ? newName : m_sCurrentScene;
        std::string objectName = newName;
        m_Scripts.Rename(oldName, newName);
        scene->SwapName(objectName); m_mapScene.swap(candidate); m_sCurrentScene.swap(active);
        m_LastError.clear(); return true;
    } catch (const std::exception& error) { m_LastError = error.what(); ENGINE_ERROR("Cannot rename Scene: {}", m_LastError); return false; }
}
std::shared_ptr<SceneObject> SceneManager::GetScene(const std::string& name) {
    auto found = m_mapScene.find(name);
    return found == m_mapScene.end() ? nullptr : found->second;
}
std::shared_ptr<SceneObject> SceneManager::GetCurrentScene() { return GetScene(m_sCurrentScene); }
bool SceneManager::SetCurrentScene(const std::string& name) {
    if (!PlaybackController::Get().CanEdit()) return false;
    if (name.empty()) { Automation::ResetSelection(); m_sCurrentScene.clear(); return true; }
    if (!m_mapScene.contains(name)) return false;
    if (name != m_sCurrentScene) Automation::ResetSelection();
    m_sCurrentScene = name;
    const auto error = SceneScriptFiles::SyntaxError(m_Scripts.ScriptPath(name));
    if (!error.empty()) ENGINE_ERROR("Scene script: {}", error);
    return true;
}
std::vector<std::string> SceneManager::GetAllSceneNames() const {
    std::vector<std::string> names; for (const auto& [name, scene] : m_mapScene) names.push_back(name); return names;
}
void SceneManager::CleanUp() { PlaybackController::Get().Stop(); Automation::ResetSelection(); m_mapScene.clear(); m_sCurrentScene.clear(); }
}
