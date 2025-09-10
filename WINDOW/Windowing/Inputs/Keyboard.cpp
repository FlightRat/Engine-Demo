#include "Keyboard.h"
#include<Logger/Logger.h>

namespace WINDOWING::INPUTS {
	Keyboard::Keyboard(): m_mapButtons{ 
						{ ENGINE_KEY_BACKSPACE, Button{} },
						{ ENGINE_KEY_TAB, Button{} },
						{ ENGINE_KEY_CLEAR, Button{} },
						{ ENGINE_KEY_RETURN, Button{} },
						{ ENGINE_KEY_PAUSE, Button{} },
						{ ENGINE_KEY_ESCAPE, Button{} },
						{ ENGINE_KEY_SPACE, Button{} },
						{ ENGINE_KEY_EXCLAIM, Button{} },
						{ ENGINE_KEY_QUOTEDBL, Button{} },
						{ ENGINE_KEY_HASH, Button{} },
						{ ENGINE_KEY_DOLLAR, Button{} },
						{ ENGINE_KEY_AMPERSAND, Button{} },
						{ ENGINE_KEY_QUOTE, Button{} },
						{ ENGINE_KEY_LEFTPAREN, Button{} },
						{ ENGINE_KEY_RIGHTPAREN, Button{} },
						{ ENGINE_KEY_ASTERISK, Button{} },
						{ ENGINE_KEY_PLUS, Button{} },
						{ ENGINE_KEY_COMMA, Button{} },
						{ ENGINE_KEY_MINUS, Button{} },
						{ ENGINE_KEY_PERIOD, Button{} },
						{ ENGINE_KEY_SLASH, Button{} },
						{ ENGINE_KEY_0, Button{} },
						{ ENGINE_KEY_1, Button{} },
						{ ENGINE_KEY_2, Button{} },
						{ ENGINE_KEY_3, Button{} },
						{ ENGINE_KEY_4, Button{} },
						{ ENGINE_KEY_5, Button{} },
						{ ENGINE_KEY_6, Button{} },
						{ ENGINE_KEY_7, Button{} },
						{ ENGINE_KEY_8, Button{} },
						{ ENGINE_KEY_9, Button{} },
						{ ENGINE_KEY_COLON, Button{} },
						{ ENGINE_KEY_SEMICOLON, Button{} },
						{ ENGINE_KEY_LESS, Button{} },
						{ ENGINE_KEY_EQUALS, Button{} },
						{ ENGINE_KEY_GREATER, Button{} },
						{ ENGINE_KEY_QUESTION, Button{} },
						{ ENGINE_KEY_AT, Button{} },
						{ ENGINE_KEY_LEFTBRACKET, Button{} },
						{ ENGINE_KEY_BACKSLASH, Button{} },
						{ ENGINE_KEY_RIGHTBRACKET, Button{} },
						{ ENGINE_KEY_CARET, Button{} },
						{ ENGINE_KEY_UNDERSCORE, Button{} },
						{ ENGINE_KEY_BACKQUOTE, Button{} },
						{ ENGINE_KEY_A, Button{} },
						{ ENGINE_KEY_B, Button{} },
						{ ENGINE_KEY_C, Button{} },
						{ ENGINE_KEY_D, Button{} },
						{ ENGINE_KEY_E, Button{} },
						{ ENGINE_KEY_F, Button{} },
						{ ENGINE_KEY_G, Button{} },
						{ ENGINE_KEY_H, Button{} },
						{ ENGINE_KEY_I, Button{} },
						{ ENGINE_KEY_J, Button{} },
						{ ENGINE_KEY_K, Button{} },
						{ ENGINE_KEY_L, Button{} },
						{ ENGINE_KEY_M, Button{} },
						{ ENGINE_KEY_N, Button{} },
						{ ENGINE_KEY_O, Button{} },
						{ ENGINE_KEY_P, Button{} },
						{ ENGINE_KEY_Q, Button{} },
						{ ENGINE_KEY_R, Button{} },
						{ ENGINE_KEY_S, Button{} },
						{ ENGINE_KEY_T, Button{} },
						{ ENGINE_KEY_U, Button{} },
						{ ENGINE_KEY_V, Button{} },
						{ ENGINE_KEY_W, Button{} },
						{ ENGINE_KEY_X, Button{} },
						{ ENGINE_KEY_Y, Button{} },
						{ ENGINE_KEY_Z, Button{} },
						{ ENGINE_KEY_DELETE, Button{} },
						{ ENGINE_KEY_CAPSLOCK, Button{} },
						{ ENGINE_KEY_F1, Button{} },
						{ ENGINE_KEY_F2, Button{} },
						{ ENGINE_KEY_F3, Button{} },
						{ ENGINE_KEY_F4, Button{} },
						{ ENGINE_KEY_F5, Button{} },
						{ ENGINE_KEY_F6, Button{} },
						{ ENGINE_KEY_F7, Button{} },
						{ ENGINE_KEY_F8, Button{} },
						{ ENGINE_KEY_F9, Button{} },
						{ ENGINE_KEY_F10, Button{} },
						{ ENGINE_KEY_F11, Button{} },
						{ ENGINE_KEY_F12, Button{} },
						{ ENGINE_KEY_SCROLLOCK, Button{} },
						{ ENGINE_KEY_PRINTSCREEN, Button{} },
						{ ENGINE_KEY_INSERT, Button{} },
						{ ENGINE_KEY_HOME, Button{} },
						{ ENGINE_KEY_PAGEUP, Button{} },
						{ ENGINE_KEY_PAGEDOWN, Button{} },
						{ ENGINE_KEY_END, Button{} },
						{ ENGINE_KEY_RIGHT, Button{} },
						{ ENGINE_KEY_LEFT, Button{} },
						{ ENGINE_KEY_DOWN, Button{} },
						{ ENGINE_KEY_UP, Button{} },
						{ ENGINE_KEY_NUMLOCK, Button{} },
						{ ENGINE_KEY_KP_DIVIDE, Button{} },
						{ ENGINE_KEY_KP_MULTIPLY, Button{} },
						{ ENGINE_KEY_KP_MINUS, Button{} },
						{ ENGINE_KEY_KP_PLUS, Button{} },
						{ ENGINE_KEY_KP_ENTER, Button{} },
						{ ENGINE_KEY_KP1, Button{} },
						{ ENGINE_KEY_KP2, Button{} },
						{ ENGINE_KEY_KP3, Button{} },
						{ ENGINE_KEY_KP4, Button{} },
						{ ENGINE_KEY_KP5, Button{} },
						{ ENGINE_KEY_KP6, Button{} },
						{ ENGINE_KEY_KP7, Button{} },
						{ ENGINE_KEY_KP8, Button{} },
						{ ENGINE_KEY_KP9, Button{} },
						{ ENGINE_KEY_KP0, Button{} },
						{ ENGINE_KEY_KP_PERIOD, Button{} },
						{ ENGINE_KEY_LCTRL, Button{} },
						{ ENGINE_KEY_LSHIFT, Button{} },
						{ ENGINE_KEY_LALT, Button{} },
						{ ENGINE_KEY_RCTRL, Button{} },
						{ ENGINE_KEY_RSHIFT, Button{} },
						{ ENGINE_KEY_RALT, Button{} } }
	{
	}

	void Keyboard::Update()
	{
		for (auto& [key, button] : m_mapButtons)
			button.Reset();
	}

	void Keyboard::OnKeyPressed(int key)
	{
		if (key == KEY_UNKNOWN)
		{
			ENGINE_ERROR("Key [{}] is unknown!", key);
			return;
		}
		auto keyItr = m_mapButtons.find(key);
		if (keyItr == m_mapButtons.end())
		{
			ENGINE_ERROR("Key [{}] does not exist!", key);
			return;
		}
		keyItr->second.Update(true);
	}

	void Keyboard::OnKeyReleased(int key)
	{
		if (key == KEY_UNKNOWN)
		{
			ENGINE_ERROR("Key [{}] is unknown!", key);
			return;
		}
		auto keyItr = m_mapButtons.find(key);
		if (keyItr == m_mapButtons.end())
		{
			ENGINE_ERROR("Key [{}] does not exist!", key);
			return;
		}
		keyItr->second.Update(false);
	}

	const bool Keyboard::IsKeyPressed(int key) const
	{
		if (key == KEY_UNKNOWN)
		{
			ENGINE_ERROR("Key [{}] is unknown!", key);
			return false;
		}
		auto keyItr = m_mapButtons.find(key);
		if (keyItr == m_mapButtons.end())
		{
			ENGINE_ERROR("Key [{}] does not exist!", key);
			return false;
		}
		return keyItr->second.bIsPressed;
	}

	const bool Keyboard::IsKeyJustPressed(int key) const
	{
		if (key == KEY_UNKNOWN)
		{
			ENGINE_ERROR("Key [{}] is unknown!", key);
			return false;
		}
		auto keyItr = m_mapButtons.find(key);
		if (keyItr == m_mapButtons.end())
		{
			ENGINE_ERROR("Key [{}] does not exist!", key);
			return false;
		}
		return keyItr->second.bJustPressed;
	}

	const bool Keyboard::IsKeyJustReleased(int key) const
	{
		if (key == KEY_UNKNOWN)
		{
			ENGINE_ERROR("Key [{}] is unknown!", key);
			return false;
		}
		auto keyItr = m_mapButtons.find(key);
		if (keyItr == m_mapButtons.end())
		{
			ENGINE_ERROR("Key [{}] does not exist!", key);
			return false;
		}
		return keyItr->second.bJustReleased;
	}
}

