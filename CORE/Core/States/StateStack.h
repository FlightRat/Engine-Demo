#pragma once
#include <vector>
#include "State.h"

namespace ENGINE_CORE {
	class StateStack
	{
	private:
		std::vector<State> m_States{};
		std::unique_ptr<State> m_pStateHolder{ nullptr };

	public:
		StateStack() = default;
		~StateStack() = default;

		void Push(State& state);
		void Pop();
		void ChangeState(State& state);

		void Update(const double dt);
		void Render();

		State& GetTop();

		static void CreateLuaStateStackBind(sol::state& lua);
	};
}