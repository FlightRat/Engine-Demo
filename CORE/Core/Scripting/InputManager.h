#pragma once
#include <Windowing/Inputs/Keyboard.h>
#include <Windowing/Inputs/Mouse.h>
#include <memory>
#include <sol/sol.hpp>

using namespace WINDOWING::INPUTS;

namespace CORE {
	class InputManager
	{
	private:
		std::unique_ptr<Keyboard> m_pKeyboard;
		std::unique_ptr<Mouse> m_pMouse;

	private:
		InputManager();
		~InputManager() = default;
		InputManager(const InputManager&) = delete;
		InputManager& operator=(const InputManager&) = delete;

	private:
		static void RegisterLuaKeyboardNames(sol::state& lua);
		static void RegisterLuaMouseNames(sol::state& lua);

	public:
		static InputManager& GetInstance();
		static void CreateLuaInputBindings(sol::state& lua);

		inline Keyboard& GetKeyBoard() { return *m_pKeyboard; }
		inline Mouse& GetMouse() { return *m_pMouse; }
	};
}