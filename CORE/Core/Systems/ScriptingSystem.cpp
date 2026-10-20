#include "ScriptingSystem.h"
#include <fstream>
#include <stdexcept>
#include "../ECS/MainRegistry.h"
#include "../ECS/Components/ScriptComponent.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/MeshFilter.h"
#include "../ECS/Components/MeshRender.h"
#include "../ECS/Components/CubeColliderComponent.h"
#include "../ECS/Components/SphereColliderComponent.h"
#include "../ECS/Components/PhysicsComponent.h"
#include "../ECS/Components/LightComponent.h"
#include "../ECS/Entity.h"
#include "../Scripting/GlmLuaBindings.h"
#include "../Scripting/SoundLuaBindings.h"
#include "../Scripting/CameraLuaBindings.h"
#include "../Scripting/UserDataLuaBindings.h"
#include "../Scripting/ContactListenerLuaBindings.h"
#include "../Inputs/InputManager.h"
#include "../CoreUtilities/FollowCamera.h"
#include "../CoreUtilities/CoreEngineData.h"
#include"../Resources/AssetManager.h"
#include"../States/State.h"
#include"../States/StateStack.h"
#include"../States/StateMachine.h"
#include<Utilities/Timer.h>
#include <Logger/Logger.h>


using namespace ENGINE_CORE::ECS;

namespace ENGINE_CORE::Systems {
    namespace {
    std::string ScriptBytes(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open Lua script");
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
    }
    bool ScriptingSystem::LoadSceneScript(sol::state& lua, const std::filesystem::path& path) {
        Release(); const auto text = path.u8string(); m_Path.assign(text.begin(), text.end());
        try {
            auto result = lua.safe_script(ScriptBytes(path), sol::script_pass_on_error, m_Path);
            if (!result.valid()) { sol::error error = result; throw error; }
            if (result.return_count() < 1) throw std::runtime_error("Scene script must return a callback table");
            sol::object object = result.get<sol::object>();
            if (!object.is<sol::table>()) throw std::runtime_error("Scene script must return a callback table");
            auto table = object.as<sol::table>();
            for (const char* name : {"on_start", "on_update", "on_stop"}) {
                sol::object value = table[name];
                if (value.get_type() != sol::type::function) throw std::runtime_error(std::string("Missing Scene function: ") + name);
            }
            m_Start = table["on_start"]; m_Update = table["on_update"]; m_Stop = table["on_stop"]; return true;
        } catch (const std::exception& error) { m_Error = m_Path + ": module: " + error.what(); return false; }
    }
    bool ScriptingSystem::Invoke(sol::protected_function& callback, const char* name) {
        if (!callback.valid()) { m_Error = m_Path + ": missing callback " + name; return false; }
        auto result = callback();
        if (result.valid()) return true;
        sol::error error = result; m_Error = m_Path + ": " + name + ": " + error.what(); return false;
    }
    bool ScriptingSystem::Start() { return Invoke(m_Start, "on_start"); }
    bool ScriptingSystem::Stop() { return Invoke(m_Stop, "on_stop"); }
    bool ScriptingSystem::Tick(double dt) {
        auto result = m_Update(dt);
        if (result.valid()) return true;
        sol::error error = result; m_Error = m_Path + ": on_update: " + error.what(); return false;
    }
    void ScriptingSystem::Release() { m_Start = sol::lua_nil; m_Update = sol::lua_nil; m_Stop = sol::lua_nil; }

	auto create_lua_logger = [](sol::state& lua) {
		auto& logger = ENGINE_LOGGER::Logger::GetInstance();

		lua.new_usertype<ENGINE_LOGGER::Logger>(
			"Logger",
			sol::no_constructor,
			"log", [&](const std::string_view message) {logger.LuaLog(message); },
			"warn", [&](const std::string_view message) {logger.LuaWarn(message); },
			"error", [&](const std::string_view message) {logger.LuaError(message); }
		);

		/*用lua中的“string.format”串联字符串，最后调用底层的Logger.log*/
		auto logResult = lua.safe_script(R"(
			function ZZZ_Log(message, ...)
				Logger.log(string.format(message, ...))
			end
		)");
		if (!logResult.valid())
		{
			ENGINE_ERROR("Failed to initialize lua logs!");
		}

		auto warnResult = lua.safe_script(R"(
			function ZZZ_Warn(message, ...)
				Logger.warn(string.format(message,...))
			end
		)");
		if (!warnResult.valid())
		{
			ENGINE_ERROR("Failed to initialize lua warnings!");
		}

		auto errorResult = lua.safe_script(R"(
			function ZZZ_Error(message,...)
				Logger.error(string.format(message,...))
			end
		)");
		if (!errorResult.valid())
		{
			ENGINE_ERROR("Failed to initialize lua errors!");
		}

		/*
		* 1.用 C++ lambda 函数来响应 Lua 的调用
		* 2.sol::variadic_args接收 Lua 传来的任意数量、任意类型的参数
		* 3.使用 try-catch 和 sol::protected_function在调用过程中捕获 C++ 或 Lua 层面抛出的异常，防止整个引擎因为一个脚本错误而崩溃。
		*/
		lua.set_function(
			"ENGINE_Log", [](const std::string& message, const sol::variadic_args& args, sol::this_state s) 
			{
				try
				{
					sol::state_view L = s;
					sol::protected_function log = L["ZZZ_Log"];
					auto result = log(message, args);
					if (!result.valid())
					{
						sol::error error = result;
						throw error;
					}
				}
				catch(const sol::error& error)
				{
					ENGINE_ERROR("Failed to get lua log: {}", error.what());
				}
			}
		);

		lua.set_function(
			"ENGINE_Warn", [](const std::string& message, const sol::variadic_args& args, sol::this_state s)
			{
				try
				{
					sol::state_view L = s;
					sol::protected_function warn = L["ZZZ_Warn"];
					auto result = warn(message, args);
					if (!result.valid())
					{
						sol::error error = result;
						throw error;
					}
				}
				catch (const sol::error& error)
				{
					ENGINE_ERROR("Failed to get lua warn: {}", error.what());
				}
			}
		);

		lua.set_function(
			"ENGINE_Error", [](const std::string& message, const sol::variadic_args& args, sol::this_state s)
			{
				try
				{
					sol::state_view L = s;
					sol::protected_function err = L["ZZZ_Error"];
					auto result = err(message, args);
					if (!result.valid())
					{
						sol::error error = result;
						throw error;
					}
				}
				catch (const sol::error& error)
				{
					ENGINE_ERROR("Failed to get lua error: {}", error.what());
				}
			}
		);

		auto assertResult = lua.safe_script(R"(
				ENGINE_assert = assert
				assert = function(arg1, message, ...)
					if not arg1 then 
						Logger.error(string.format(message, ...))
					end 
					ENGINE_assert(arg1)
				end
			)");
	};

	void ScriptingSystem::RegisterLuaBindings(sol::state& lua, ENGINE_CORE::ECS::Registry& registry)
	{
		// many lua registey
		Registry::CreateLuaRegistryBind(lua, registry);
		ENGINE_CORE::Scripting::GLMBindings::CreateLuaGlmBind(lua);
		ENGINE_CORE::Scripting::SoundBindings::CreateLuaSoundBind(lua, registry);
		ENGINE_CORE::Scripting::CameraBindings::CreateLuaCameraBind(lua, registry);
		ENGINE_CORE::Scripting::UserDataBindings::CreateLuaUserDataBind(lua);
		ENGINE_CORE::Scripting::ContactListenerBindings::CreateLuaContactListenerBind(lua, registry);
		ENGINE_CORE::FollowCamera::CreateLuaFollowCameraBind(lua, registry);
		auto* runtimePtr = registry.GetRegistry().ctx().find<std::shared_ptr<GameplayContext>>();
        const auto runtime = runtimePtr ? *runtimePtr : nullptr;
        ENGINE_CORE::INPUTS::InputManager::CreateLuaInputBind(lua, runtime);
		ENGINE_CORE::State::CreateLuaStateBind(lua);
		ENGINE_CORE::StateStack::CreateLuaStateStackBind(lua);
		ENGINE_CORE::StateMachine::CreateLuaStateMachineBind(lua);
		ENGINE_CORE::RESOURCES::AssetManager::CreateLuaAssetManagerBind(lua, registry);
		ENGINE_UTIL::Timer::CreateLuaTimerBind(lua, runtime ? std::function<int64_t()>([runtime] { return int64_t(runtime->clock.Milliseconds()); }) : std::function<int64_t()>{});

		create_lua_logger(lua);

		// register components into lua
		ENGINE_CORE::ECS::Entity::CreateLuaEntityBind(lua, registry);				
		TransformComponent::CreateLuaTransformBind(lua);
		CubeColliderComponent::CreateLuaCubeColliderBind(lua);
		SphereColliderComponent::CreateLuaSphereColliderBind(lua);
		MeshFilter::CreateLuaMeshFilterBind(lua);
		MeshRender::CreateLuaMeshRendererBind(lua);
		PhysicsComponent::CreateLuaPhysicsBind(lua, registry.GetRegistry());
		LightComponent::CreateLuaLightBind(lua);

        RegisterComponentMetadata();
	}

    void ScriptingSystem::RegisterComponentMetadata() {
		//NOTE::the same registered Component in LUA and META should have the same id

		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<TransformComponent>();	// register TransformComponent into meta
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<MeshFilter>();
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<MeshRender>();
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<CubeColliderComponent>();
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<SphereColliderComponent>();
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<PhysicsComponent>();
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<LightComponent>();

		Registry::RegisterMetaComponent<TransformComponent>();
		Registry::RegisterMetaComponent<MeshFilter>();
		Registry::RegisterMetaComponent<MeshRender>();
		Registry::RegisterMetaComponent<CubeColliderComponent>();
		Registry::RegisterMetaComponent<SphereColliderComponent>();
		Registry::RegisterMetaComponent<PhysicsComponent>();
		Registry::RegisterMetaComponent<LightComponent>();

		ENGINE_CORE::Scripting::UserDataBindings::register_meta_user_data<ENGINE_PHYSICS::ObjectData>();
    }

    void ScriptingSystem::RegisterLuaFunctions(sol::state& lua, std::shared_ptr<GameplayContext> runtime) {
        lua.set_function("ENGINE_RunScript", [&lua](const std::string& path) {
            auto result = lua.safe_script(ScriptBytes(std::filesystem::path(std::u8string(path.begin(), path.end()))), sol::script_pass_on_error, path);
            if (!result.valid()) { sol::error error = result; throw error; }
            return true;
        });
        lua.set_function("ENGINE_LoadScriptTable", [&lua](const sol::table& list) {
            if (!list.valid()) throw std::runtime_error("Invalid script list");
            for (const auto& item : list) {
                const auto path = item.second.as<std::string>();
                auto result = lua.safe_script(ScriptBytes(std::filesystem::path(std::u8string(path.begin(), path.end()))), sol::script_pass_on_error, path);
                if (!result.valid()) { sol::error error = result; throw error; }
            }
        });
        auto& engine = CoreEngineData::GetInstance();
        lua.set_function("ENGINE_GetTicks", [runtime] { return runtime ? runtime->clock.Milliseconds() : uint64_t(SDL_GetTicks()); });
        lua.set_function("ENGINE_DeltaTime", [runtime, &engine] { return runtime ? runtime->clock.Delta() : engine.GetDeltaTime(); });
        lua.set_function("ENGINE_WindowWidth", [&engine] { return engine.WindowWidth(); });
        lua.set_function("ENGINE_WindowHeight", [&engine] { return engine.WindowHeight(); });
        lua.set_function("ENGINE_DisablePhysics", [runtime, &engine] { if (runtime) runtime->physicsEnabled = false; else engine.DisablePhysics(); });
        lua.set_function("ENGINE_EnablePhysics", [runtime, &engine] { if (runtime) runtime->physicsEnabled = true; else engine.EnablePhysics(); });
        lua.set_function("ENGINE_IsPhysicsEnabled", [runtime, &engine] { return runtime ? runtime->physicsEnabled : engine.IsPhysicsEnabled(); });
        lua.set_function("ENGINE_DisableCollisionRendering", [runtime, &engine] { if (runtime) runtime->renderColliders = false; else engine.DisableColliderRender(); });
        lua.set_function("ENGINE_EnableCollisionRendering", [runtime, &engine] { if (runtime) runtime->renderColliders = true; else engine.EnableColliderRender(); });
        lua.set_function("ENGINE_CollisionRenderingEnabled", [runtime, &engine] { return runtime ? runtime->renderColliders : engine.RenderCollidersEnabled(); });
        lua.set_function("ENGINE_GetProjecPath", [&engine] { return engine.GetProjectPath(); });
    }
}
