#include "InputManager.h"
#include <glm/glm.hpp>


namespace ENGINE_CORE::INPUTS {
	InputManager::InputManager():m_pKeyboard{std::make_unique<Keyboard>()}, m_pMouse{ std::make_unique<Mouse>() }
	{

	}

	/*register key into lua*/
	void InputManager::RegisterLuaKeyboardNames(sol::state& lua)
	{
		lua.set("KEY_A", ENGINE_KEY_A);
		lua.set("KEY_B", ENGINE_KEY_B);
		lua.set("KEY_C", ENGINE_KEY_C);
		lua.set("KEY_D", ENGINE_KEY_D);
		lua.set("KEY_E", ENGINE_KEY_E);
		lua.set("KEY_F", ENGINE_KEY_F);
		lua.set("KEY_G", ENGINE_KEY_G);
		lua.set("KEY_H", ENGINE_KEY_H);
		lua.set("KEY_I", ENGINE_KEY_I);
		lua.set("KEY_J", ENGINE_KEY_J);
		lua.set("KEY_K", ENGINE_KEY_K);
		lua.set("KEY_L", ENGINE_KEY_L);
		lua.set("KEY_M", ENGINE_KEY_M);
		lua.set("KEY_N", ENGINE_KEY_N);
		lua.set("KEY_O", ENGINE_KEY_O);
		lua.set("KEY_P", ENGINE_KEY_P);
		lua.set("KEY_Q", ENGINE_KEY_Q);
		lua.set("KEY_R", ENGINE_KEY_R);
		lua.set("KEY_S", ENGINE_KEY_S);
		lua.set("KEY_T", ENGINE_KEY_T);
		lua.set("KEY_U", ENGINE_KEY_U);
		lua.set("KEY_V", ENGINE_KEY_V);
		lua.set("KEY_W", ENGINE_KEY_W);
		lua.set("KEY_X", ENGINE_KEY_X);
		lua.set("KEY_Y", ENGINE_KEY_Y);
		lua.set("KEY_Z", ENGINE_KEY_Z);

		lua.set("KEY_0", ENGINE_KEY_0);
		lua.set("KEY_1", ENGINE_KEY_1);
		lua.set("KEY_2", ENGINE_KEY_2);
		lua.set("KEY_3", ENGINE_KEY_3);
		lua.set("KEY_4", ENGINE_KEY_4);
		lua.set("KEY_5", ENGINE_KEY_5);
		lua.set("KEY_6", ENGINE_KEY_6);
		lua.set("KEY_7", ENGINE_KEY_7);
		lua.set("KEY_8", ENGINE_KEY_8);
		lua.set("KEY_9", ENGINE_KEY_9);

		lua.set("KEY_ENTER", ENGINE_KEY_RETURN);
		lua.set("KEY_BACKSPACE", ENGINE_KEY_BACKSPACE);
		lua.set("KEY_ESC", ENGINE_KEY_ESCAPE);
		lua.set("KEY_SPACE", ENGINE_KEY_SPACE);
		lua.set("KEY_LCTRL", ENGINE_KEY_LCTRL);
		lua.set("KEY_RCTRL", ENGINE_KEY_RCTRL);
		lua.set("KEY_LALT", ENGINE_KEY_LALT);
		lua.set("KEY_RALT", ENGINE_KEY_RALT);
		lua.set("KEY_LSHIFT", ENGINE_KEY_LSHIFT);
		lua.set("KEY_RSHIFT", ENGINE_KEY_RSHIFT);

		// ==================================================================
		//  Register Punctuation Keys
		// ==================================================================
		lua.set("KEY_COLON", ENGINE_KEY_COLON);
		lua.set("KEY_SEMICOLON", ENGINE_KEY_SEMICOLON);
		lua.set("KEY_QUOTE", ENGINE_KEY_QUOTE);
		lua.set("KEY_BACKQUOTE", ENGINE_KEY_BACKQUOTE);
		lua.set("KEY_CARET", ENGINE_KEY_CARET);
		lua.set("KEY_UNDERSCORE", ENGINE_KEY_UNDERSCORE);
		lua.set("KEY_RIGHTBRACKET", ENGINE_KEY_RIGHTBRACKET);
		lua.set("KEY_LEFTBRACKET", ENGINE_KEY_LEFTBRACKET);
		lua.set("KEY_SLASH", ENGINE_KEY_SLASH);
		lua.set("KEY_ASTERISK", ENGINE_KEY_ASTERISK);
		lua.set("KEY_LEFTPAREN", ENGINE_KEY_LEFTPAREN);
		lua.set("KEY_RIGHTPAREN", ENGINE_KEY_RIGHTPAREN);
		lua.set("KEY_QUESTION", ENGINE_KEY_QUESTION);
		lua.set("KEY_AMPERSAND", ENGINE_KEY_AMPERSAND);
		lua.set("KEY_DOLLAR", ENGINE_KEY_DOLLAR);
		lua.set("KEY_EXCLAIM", ENGINE_KEY_EXCLAIM);
		lua.set("KEY_BACKSLASH", ENGINE_KEY_BACKSLASH);

		// ==================================================================
		//  Register Function Keys
		// ==================================================================
		lua.set("KEY_F1", ENGINE_KEY_F1);
		lua.set("KEY_F2", ENGINE_KEY_F2);
		lua.set("KEY_F3", ENGINE_KEY_F3);
		lua.set("KEY_F4", ENGINE_KEY_F4);
		lua.set("KEY_F5", ENGINE_KEY_F5);
		lua.set("KEY_F6", ENGINE_KEY_F6);
		lua.set("KEY_F7", ENGINE_KEY_F7);
		lua.set("KEY_F8", ENGINE_KEY_F8);
		lua.set("KEY_F9", ENGINE_KEY_F9);
		lua.set("KEY_F10", ENGINE_KEY_F10);
		lua.set("KEY_F11", ENGINE_KEY_F11);
		lua.set("KEY_F12", ENGINE_KEY_F12);

		// ==================================================================
		// Register Cursor Control Keys
		// ==================================================================
		lua.set("KEY_UP", ENGINE_KEY_UP);
		lua.set("KEY_RIGHT", ENGINE_KEY_RIGHT);
		lua.set("KEY_DOWN", ENGINE_KEY_DOWN);
		lua.set("KEY_LEFT", ENGINE_KEY_LEFT);

		// ==================================================================
		// Register Numeric Keypad Keys
		// ==================================================================
		lua.set("KP_KEY_0", ENGINE_KEY_KP0);
		lua.set("KP_KEY_1", ENGINE_KEY_KP1);
		lua.set("KP_KEY_2", ENGINE_KEY_KP2);
		lua.set("KP_KEY_3", ENGINE_KEY_KP3);
		lua.set("KP_KEY_4", ENGINE_KEY_KP4);
		lua.set("KP_KEY_5", ENGINE_KEY_KP5);
		lua.set("KP_KEY_6", ENGINE_KEY_KP6);
		lua.set("KP_KEY_7", ENGINE_KEY_KP7);
		lua.set("KP_KEY_8", ENGINE_KEY_KP8);
		lua.set("KP_KEY_9", ENGINE_KEY_KP9);

		lua.set("KP_KEY_DIVIDE", ENGINE_KEY_KP_DIVIDE);
		lua.set("KP_KEY_MULTIPLY", ENGINE_KEY_KP_MULTIPLY);
		lua.set("KP_KEY_MINUS", ENGINE_KEY_KP_MINUS);
		lua.set("KP_KEY_PLUS", ENGINE_KEY_KP_PLUS);
		lua.set("KP_KEY_ENTER", ENGINE_KEY_KP_ENTER);
		lua.set("KP_KEY_PERIOD", ENGINE_KEY_KP_PERIOD);
	}

	void InputManager::RegisterLuaMouseNames(sol::state& lua)
	{
		lua.set("MOUSE_LEFT", ENGINE_MOUSE_LEFT);
		lua.set("MOUSE_MIDDLE", ENGINE_MOUSE_MIDDLE);
		lua.set("MOUSE_RIGHT", ENGINE_MOUSE_RIGHT);
	}

	InputManager& InputManager::GetInstance()
	{
		static InputManager instance{};
		return instance;
	}

	void InputManager::CreateLuaInputBind(sol::state& lua)
	{
		RegisterLuaKeyboardNames(lua);	// register keyboard btn into lua
		RegisterLuaMouseNames(lua);		// register mouse btn into lua

		auto& inputManager = GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		auto& mouse = inputManager.GetMouse();

		// register keyboard into lua
		lua.new_usertype<Keyboard>(
			"Keyboard",
			sol::no_constructor,
			"just_pressed", [&](int key) {return keyboard.IsKeyJustPressed(key); },
			"just_released", [&](int key) {return keyboard.IsKeyJustReleased(key); },
			"pressed", [&](int key) {return keyboard.IsKeyPressed(key); }
		);

		// register mouse into lua
		lua.new_usertype<Mouse>(
			"Mouse",
			sol::no_constructor,
			"pressed", [&](int btn) {return mouse.IsBtnPressed(btn); },
			"just_pressed", [&](int btn) {return mouse.IsBtnJustPressed(btn); },
			"just_released", [&](int btn) {return mouse.IsBtnJustReleased(btn); },
			//"screen_pos", [&]() {return mouse.GetMouseScreenPosition(); },
			"offset", [&]() {
				auto [x, y] = mouse.GetMouseOffset();
				float xrel = static_cast<float>(x);
				float yrel = static_cast<float>(y);
				return glm::vec2(xrel, yrel); },
			"wheel_x", [&]() {return static_cast<float>(mouse.GetMouseWheelX()); },
			"wheel_y", [&]() {return static_cast<float>(mouse.GetMouseWheelY()); },
			"moving", [&]() {return mouse.IsMouseMoving(); }
		);
	}
}


