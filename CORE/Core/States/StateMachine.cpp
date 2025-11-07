#include "StateMachine.h"
#include "Logger/Logger.h"

namespace ENGINE_CORE {
	StateMachine::StateMachine():StateMachine(sol::lua_nil_t{})
	{
	}

	StateMachine::StateMachine(const sol::table& stateFuncs):
		m_mapStates{},
		m_sCurrentState{},
		m_StateTable{ stateFuncs }
	{
	}

	/*	
	1.find newState
	2.call the onExit of oldState & mark it as kill, and mark newState as currentState
	3.call the onEnter of newState
	*/
	void StateMachine::ChangeStates(const std::string& stateName, bool bRemoveState, const sol::object& enterParams)
	{	
		auto stateItr = m_mapStates.find(stateName);
		if (stateItr == m_mapStates.end())
		{
			ENGINE_ERROR("Failed to change state -- [{}] does not exist!", stateName);
			return;
		}
		auto& newState = stateItr->second;
		
		if (m_sCurrentState.empty())
		{
			m_sCurrentState = stateName;
		}
		else
		{
			auto& oldState = m_mapStates.at(m_sCurrentState);
			if (oldState->on_exit.valid())
			{
				try
				{
					auto result = oldState->on_exit();
					if (!result.valid())
					{
						sol::error error = result;
						throw  error;
					}
				}
				catch (const sol::error& error)
				{
					ENGINE_ERROR("Failed to exit state: {}", error.what());
					return;
				}
			}
			if (bRemoveState)
				oldState->bKillState = true;
			m_sCurrentState = stateName;
		}

		if (newState->on_enter.valid())
		{
			try
			{
				auto result = newState->on_enter();
				if (!result.valid())
				{
					sol::error error = result;
					throw  error;
				}
			}
			catch (const sol::error& error)
			{
				ENGINE_ERROR("Failed to enter state: {}", error.what());
				return;
			}
		}
	}

	/*call the onUpdate of currentState, and erase states marked as kill*/
	void StateMachine::Update(const float dt)
	{
		try
		{
			auto stateItr = m_mapStates.find(m_sCurrentState);
			if (stateItr == m_mapStates.end())
				return;

			auto& currentState = stateItr->second;
			if (currentState->on_update.valid())
			{
				auto result = currentState->on_update(dt);
				if (!result.valid())
				{
					sol::error error = result;
					throw  error;
				}
			}
			std::erase_if(m_mapStates, [](auto& state) { return state.second->bKillState; });
		}
		catch (const sol::error& error)
		{
			ENGINE_ERROR("Failed to update state: {}", error.what());
		}
		catch (...)
		{
			ENGINE_ERROR("Failed to update state: Unknown error!}");
		}
	}

	/*call the on_render of currentState*/
	void StateMachine::Render()
	{
		try
		{
			auto stateItr = m_mapStates.find(m_sCurrentState);
			if (stateItr == m_mapStates.end())
				return;

			auto& currentState = stateItr->second;
			if (currentState->on_render.valid())
			{
				auto result = currentState->on_render();
				if (!result.valid())
				{
					sol::error error = result;
					throw  error;
				}
			}
		}
		catch (const sol::error& error)
		{
			ENGINE_ERROR("Failed to update state: {}", error.what());
		}
		catch (...)
		{
			ENGINE_ERROR("Failed to render state: Unknown error!}");
		}
	}

	/* add new state to the map*/
	void StateMachine::AddState(const State& state)
	{
		if (m_mapStates.contains(state.name))
		{
			ENGINE_ERROR("Failed to add state: {} -- Already exists!", state.name);
			return;
		}
		m_mapStates.emplace(state.name, std::make_unique<State>(state));
	}

	/* call the on_exit of currentState and mark as kill*/
	void StateMachine::ExitState()
	{
		auto stateItr = m_mapStates.find(m_sCurrentState);
		if (stateItr == m_mapStates.end())
		{
			ENGINE_ERROR("Failed to exit state: {} -- Does not exist!", m_sCurrentState);
			return;
		}
		stateItr->second->on_exit();
		stateItr->second->bKillState = true;
		m_sCurrentState.clear();
	}

	/*exit all states and clear*/
	void StateMachine::DestroyStates()
	{
		for (auto& [name, state] : m_mapStates)
		{
			state->on_exit();
		}
		m_mapStates.clear();
	}

	void StateMachine::CreateLuaStateMachineBind(sol::state& lua)
	{
		lua.new_usertype<StateMachine>(
			"StateMachine",
			sol::call_constructor,
			sol::constructors<StateMachine(), StateMachine(const sol::table&)>(),
			"change_state", sol::overload(
				[](StateMachine& sm, const std::string& state, bool bRemove, const sol::object& enterParams) {
					sm.ChangeStates(state, bRemove, enterParams);
				},
				[](StateMachine& sm, const std::string& state, bool bRemove) {
					sm.ChangeStates(state, bRemove);
				},
				[](StateMachine& sm, const std::string& state) {
					sm.ChangeStates(state);
				}
			),
			"update", &StateMachine::Update,
			"render", &StateMachine::Render,
			"current_state", &StateMachine::CurrentState,
			"add_state", &StateMachine::AddState,
			"exit_state", &StateMachine::ExitState,
			"destroy", &StateMachine::DestroyStates
		);
	}
}


