#pragma once
#include <sol/sol.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include "../CoreUtilities/GameplayContext.h"
namespace ENGINE_CORE::ECS { class Registry; }
namespace ENGINE_CORE::Systems {
class ScriptingSystem {
    sol::protected_function m_Start, m_Update, m_Stop;
    std::string m_Path, m_Error;
    bool Invoke(sol::protected_function& callback, const char* name);
public:
    ScriptingSystem() = default;
    bool LoadSceneScript(sol::state& lua, const std::filesystem::path& path);
    bool Start();
    bool Tick(double dt);
    bool Stop();
    void Release();
    const std::string& LastError() const { return m_Error; }
    static void RegisterComponentMetadata();
    static void RegisterLuaBindings(sol::state& lua, ECS::Registry& registry);
    static void RegisterLuaFunctions(sol::state& lua, std::shared_ptr<GameplayContext> runtime = {});
};
}
