#include "ScriptingSystem.h"
#include "../ECS/Components/ScriptComponent.h"
#include "../ECS/Entity.h"
#include <Logger/Logger.h>

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

		sol::table main_lua = lua["main"];

		sol::optional<sol::table> bUpdateExists = main_lua[1];
		if (bUpdateExists == sol::nullopt)
		{
			ENGINE_ERROR("There is no update function in main.lua!");
			return false;
		}
		sol::table update_script = main_lua[1];
		sol::function update = update_script["update"];

		sol::optional<sol::table> bRenderExists = main_lua[2];
		if (bRenderExists = sol::nullopt)
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

}

