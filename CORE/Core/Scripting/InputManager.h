#pragma once
#include <Windowing/Inputs/Keyboard.h>
#include <Windowing/Inputs/Mouse.h>
#include <memory>
#include <sol/sol.hpp>

using namespace ENGINE_WINDOWING::INPUTS;

/*
整体思路：
	用一个static InputManager存储、注册、获取“鼠标”和“键盘”
	一个“鼠标”/“键盘”用一个map表示，用于记录每一个按键的状态（“鼠标”包含额外的滚轮和移动）；并对外暴露“查询按键状态”和“修改按键状态”的函数
	在Application.cpp的ProcessEvents函数中用SDL检测“鼠标”/“键盘”的输入，调用函数修改“鼠标”/“键盘”对应按键的状态
	“鼠标”/“键盘”会被注册到lua，并包含获取“鼠标”/“键盘”状态的函数；lua脚本中会调用这些函数，查询对于输入，然后再做下一步操作

	SDL的“鼠标”/“键盘”代码（“SDLK_a”）会被constexpr常量重新定义为引擎的按键代码（“ENGINE_KEY_A”）
	这些代码会被set到lua中（lua.set("KEY_A", ENGINE_KEY_A);），也会用在“鼠标”/“键盘”的map中
	"ENGINE_KEY_A" == "SDLK_a"
	状态查询：LUA中查询“KEY_A”的状态，即查询map中“ENGINE_KEY_A”的状态
	状态修改：检测到"SDLK_a"输入，修改map中"SDLK_a"，即“ENGINE_KEY_A”的状态
*/

namespace ENGINE_CORE {
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
		static void CreateLuaInputBind(sol::state& lua);

		inline Keyboard& GetKeyBoard() { return *m_pKeyboard; }
		inline Mouse& GetMouse() { return *m_pMouse; }
	};
}