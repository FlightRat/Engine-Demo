#include "ScriptingSystem.h"
#include "../ECS/Components/ScriptComponent.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/MeshFilter.h"
#include "../ECS/Components/MeshRender.h"
#include "../ECS/Components/CubeColliderComponent.h"
#include "../ECS/Components/SphereColliderComponent.h"
#include "../ECS/Components/PhysicsComponent.h"
#include "../ECS/Entity.h"
#include "../Scripting/GlmLuaBindings.h"
#include "../Scripting/SoundLuaBindings.h"
#include "../Scripting/CameraLuaBindings.h"
#include "../Scripting/UserDataLuaBindings.h"
#include "../Scripting/ContactListenerLuaBindings.h"
#include "../Scripting/InputManager.h"
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
	ScriptingSystem::ScriptingSystem(ENGINE_CORE::ECS::Registry& registry):m_Registry{ registry },m_bMainLoaded{false}
	{
	}

	bool ScriptingSystem::LoadMainScript(sol::state& lua)
	{
		// read the lua script
		try
		{
			auto result = lua.safe_script_file("./assets/scripts/main.lua");
		}
		catch(const sol::error& err)
		{
			ENGINE_ERROR("Error loading the main lua script: {0}", err.what());
			return false;
		}

		sol::table main_lua = lua["main"];	// read the "main" into sol::table main_lua

		sol::optional<sol::table> bUpdateExists = main_lua[1]; // check if the element 1 is a "sol::table"
		if (bUpdateExists == sol::nullopt)
		{
			ENGINE_ERROR("There is no update function in main.lua!");
			return false;
		}
		sol::table update_script = main_lua[1];
		sol::function update = update_script["update"];

		sol::optional<sol::table> bRenderExists = main_lua[2];
		if (bRenderExists == sol::nullopt)
		{
			ENGINE_ERROR("There is no render function in main.lua!");
			return false;
		}
		sol::table render_script = main_lua[2];
		sol::function render = render_script["render"];

		ENGINE_CORE::ECS::Entity mainLuaScript{ m_Registry, "main_script", "" };
		mainLuaScript.AddComponent<ENGINE_CORE::ECS::ScriptComponent>(
			ENGINE_CORE::ECS::ScriptComponent{
				.update = update,
				.render = render,
			}
		);

		m_bMainLoaded = true;

		return true;
	}

	void ScriptingSystem::Update()
	{
		if (!m_bMainLoaded)
		{
			ENGINE_ERROR("Main lua script has not been loaded!");
			return;
		}

		auto view = m_Registry.GetRegistry().view<ENGINE_CORE::ECS::ScriptComponent>();

		for (const auto& entity : view) 
		{
			ENGINE_CORE::ECS::Entity ent{ m_Registry, entity };
			if (ent.GetName() != "main_script")
				continue;
			auto& script = ent.GetComponent<ENGINE_CORE::ECS::ScriptComponent>();
			auto error = script.update(entity);
			if (!error.valid())
			{
				sol::error err = error;
				ENGINE_ERROR("Error running the Update script： {0}", err.what());
			}
		}

		auto& lua = m_Registry.GetContext<std::shared_ptr<sol::state>>();
		if (lua)
		{
			lua->collect_garbage();
		}
	}

	void ScriptingSystem::Render()
	{
		if (!m_bMainLoaded)
		{
			ENGINE_ERROR("Main lua script has not been loaded!");
			return;
		}

		auto view = m_Registry.GetRegistry().view<ENGINE_CORE::ECS::ScriptComponent>();

		for (const auto& entity : view)
		{
			ENGINE_CORE::ECS::Entity ent{ m_Registry, entity };
			if (ent.GetName() != "main_script")
				continue;
			auto& script = ent.GetComponent<ENGINE_CORE::ECS::ScriptComponent>();
			auto error = script.render(entity);
			if (!error.valid())
			{
				sol::error err = error;
				ENGINE_ERROR("Error running the Render script： {0}", err.what());
			}
		}

		auto& lua = m_Registry.GetContext<std::shared_ptr<sol::state>>();
		if (lua)
		{
			lua->collect_garbage();
		}
	}

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
		ENGINE_CORE::InputManager::CreateLuaInputBind(lua);
		ENGINE_CORE::State::CreateLuaStateBind(lua);
		ENGINE_CORE::StateStack::CreateLuaStateStackBind(lua);
		ENGINE_CORE::StateMachine::CreateLuaStateMachineBind(lua);
		ENGINE_RESOURCES::AssetManager::CreateLuaAssetManagerBind(lua, registry);
		ENGINE_UTIL::Timer::CreateLuaTimerBind(lua);

		create_lua_logger(lua);

		// register components into lua
		ENGINE_CORE::ECS::Entity::CreateLuaEntityBind(lua, registry);				
		TransformComponent::CreateLuaTransformBind(lua);
		CubeColliderComponent::CreateLuaCubeColliderBind(lua);
		SphereColliderComponent::CreateLuaSphereColliderBind(lua);
		MeshFilter::CreateLuaMeshFilterBind(lua);
		MeshRender::CreateLuaMeshRendererBind(lua);
		PhysicsComponent::CreateLuaPhysicsBind(lua, registry.GetRegistry());

		//NOTE::the same registered Component in LUA and META should have the same id

		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<TransformComponent>();	// register TransformComponent into meta
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<MeshFilter>();
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<MeshRender>();
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<CubeColliderComponent>();
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<SphereColliderComponent>();
		ENGINE_CORE::ECS::Entity::RegisterMetaComponent<PhysicsComponent>();

		Registry::RegisterMetaComponent<TransformComponent>();
		Registry::RegisterMetaComponent<MeshFilter>();
		Registry::RegisterMetaComponent<MeshRender>();
		Registry::RegisterMetaComponent<CubeColliderComponent>();
		Registry::RegisterMetaComponent<SphereColliderComponent>();
		Registry::RegisterMetaComponent<PhysicsComponent>();

		ENGINE_CORE::Scripting::UserDataBindings::register_meta_user_data<ENGINE_PHYSICS::ObjectData>();
	}

	void ScriptingSystem::RegisterLuaFunctions(sol::state& lua)
	{
		lua.set_function(
			"ENGINE_RunScript", [&](const std::string& path)
			{
				try
				{
					lua.safe_script_file(path);
				}
				catch (const sol::error& error)
				{
					ENGINE_ERROR("Error loading Lua Script:{}", error.what());
					return false;
				}
				return true;
			}
		);

		lua.set_function(
			"ENGINE_LoadScriptTable", [&](const sol::table& scriptList) {
				if (!scriptList.valid())
				{
					ENGINE_ERROR("Failed to load script list: invalid!");
					return;
				}
				for (const auto& [index, script] : scriptList)
				{
					try
					{
						auto result = lua.safe_script_file(script.as<std::string>());
						if (!result.valid())
						{
							sol::error error = result;
							throw error;
						}
					}
					catch(const sol::error& error)
					{
						ENGINE_ERROR("Failed to load script: {}, Error: {}", script.as<std::string>(), error.what());
						return;
					}
				}
			}
		);

		lua.set_function("ENGINE_GetTicks", [] { return SDL_GetTicks(); }); // todo:where to use???

		auto& engine = CoreEngineData::GetInstance();
		lua.set_function("ENGINE_DeltaTime", [&] { return engine.GetDeltaTime(); });
		lua.set_function("ENGINE_WindowWidth", [&] { return engine.WindowWidth(); });
		lua.set_function("ENGINE_WindowHeight", [&] { return engine.WindowHeight(); });
		// Physics Enable functions
		lua.set_function("ENGINE_DisablePhysics", [&] { engine.DisablePhysics(); });
		lua.set_function("ENGINE_EnablePhysics", [&] { engine.EnablePhysics(); });
		lua.set_function("ENGINE_IsPhysicsEnabled", [&] { return engine.IsPhysicsEnabled(); });
		// Render Colliders Enable functions
		lua.set_function("ENGINE_DisableCollisionRendering", [&] { engine.DisableColliderRender(); });
		lua.set_function("ENGINE_EnableCollisionRendering", [&] { engine.EnableColliderRender(); });
		lua.set_function("ENGINE_CollisionRenderingEnabled", [&] { return engine.RenderCollidersEnabled(); });
		// Path
		lua.set_function("ENGINE_GetProjecPath", [&] { return engine.GetProjectPath(); });
	}
}

