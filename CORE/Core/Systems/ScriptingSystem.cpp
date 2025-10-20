#include "ScriptingSystem.h"
#include "../ECS/Components/ScriptComponent.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/MeshFilter.h"
#include "../ECS/Components/MeshRender.h"
#include "../ECS/Components/CubeColliderComponent.h"
#include "../ECS/Components/SphereColliderComponent.h"
#include "../ECS/Components/PhysicsComponent.h"
#include "../ECS/Entity.h"
#include <Logger/Logger.h>
#include "../Scripting/GlmLuaBindings.h"
#include "../Scripting/SoundLuaBindings.h"
#include "../Scripting/InputManager.h"
#include"../Resources/AssetManager.h"
#include<Utilities/Timer.h>

using namespace CORE::ECS;

namespace CORE::Systems {
	ScriptingSystem::ScriptingSystem(CORE::ECS::Registry& registry):m_Registry{ registry },m_bMainLoaded{false}
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

		CORE::ECS::Entity mainLuaScript{ m_Registry, "main_script", "" };
		mainLuaScript.AddComponent<CORE::ECS::ScriptComponent>(
			CORE::ECS::ScriptComponent{
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

		auto view = m_Registry.GetRegistry().view<CORE::ECS::ScriptComponent>();

		for (const auto& entity : view) 
		{
			CORE::ECS::Entity ent{ m_Registry, entity };
			if (ent.GetName() != "main_script")
				continue;
			auto& script = ent.GetComponent<CORE::ECS::ScriptComponent>();
			auto error = script.update(entity);
			if (!error.valid())
			{
				sol::error err = error;
				ENGINE_ERROR("Error running the Update script£º {0}", err.what());
			}
		}
	}

	void ScriptingSystem::Render()
	{
		if (!m_bMainLoaded)
		{
			ENGINE_ERROR("Main lua script has not been loaded!");
			return;
		}

		auto view = m_Registry.GetRegistry().view<CORE::ECS::ScriptComponent>();

		for (const auto& entity : view)
		{
			CORE::ECS::Entity ent{ m_Registry, entity };
			if (ent.GetName() != "main_script")
				continue;
			auto& script = ent.GetComponent<CORE::ECS::ScriptComponent>();
			auto error = script.render(entity);
			if (!error.valid())
			{
				sol::error err = error;
				ENGINE_ERROR("Error running the Render script£º {0}", err.what());
			}
		}
	}

	void ScriptingSystem::RegisterLuaBindings(sol::state& lua, CORE::ECS::Registry& registry)
	{
		Registry::CreateLuaRegistryBind(lua, registry);							// register "runtime_view" & "Registry" into lua
		CORE::Scripting::GLMBindings::CreateGLMBindings(lua);					// register glm vec into lua
		CORE::InputManager::CreateLuaInputBindings(lua);						// register inputs stuff into lua
		RESOURCES::AssetManager::CreateLuaAssetManager(lua, registry);			// register assetManager into lua
		CORE::Scripting::SoundBindings::CreateSoundBindings(lua, registry);		// register sound into lua
		UTIL::Timer::CreateLuaTimer(lua);										// register timer into lua

		CORE::ECS::Entity::CreateLuaEntityBind(lua, registry);				// register a "Entity" type into lua
		TransformComponent::CreateLuaTransformBind(lua);		// register a "Transform" type into lua
		CubeColliderComponent::CreateLuaCubeColliderBind(lua);
		SphereColliderComponent::CreateLuaSphereColliderBind(lua);
		MeshFilter::CreateLuaMeshFilterBind(lua);
		MeshRender::CreateLuaMeshRendererBind(lua);
		PhysicsComponent::CreateLuaPhysicsBind(lua, registry.GetRegistry());

		//NOTE::the same registered Component in LUA and META should have the same id

		CORE::ECS::Entity::RegisterMetaComponent<TransformComponent>();	// register TransformComponent into meta
		CORE::ECS::Entity::RegisterMetaComponent<MeshFilter>();
		CORE::ECS::Entity::RegisterMetaComponent<MeshRender>();
		CORE::ECS::Entity::RegisterMetaComponent<CubeColliderComponent>();
		CORE::ECS::Entity::RegisterMetaComponent<SphereColliderComponent>();
		CORE::ECS::Entity::RegisterMetaComponent<PhysicsComponent>();

		Registry::RegisterMetaComponent<TransformComponent>();
		Registry::RegisterMetaComponent<MeshFilter>();
		Registry::RegisterMetaComponent<MeshRender>();
		Registry::RegisterMetaComponent<CubeColliderComponent>();
		Registry::RegisterMetaComponent<SphereColliderComponent>();
		Registry::RegisterMetaComponent<PhysicsComponent>();
	}

	void ScriptingSystem::RegisterLuaFunctions(sol::state& lua)
	{
		lua.set_function(
			"run_script", [&](const std::string& path)
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
	}
}

