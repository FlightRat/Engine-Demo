#pragma once
#include "Button.h"
#include "MouseButtons.h"
#include <map>

namespace ENGINE_WINDOWING::INPUTS {
	class Mouse
	{
	private:
		std::map<int, Button> m_mapButtons
		{
			{ENGINE_MOUSE_LEFT, Button{}},
			{ENGINE_MOUSE_MIDDLE, Button{}},
			{ENGINE_MOUSE_RIGHT, Button{}},
		};
		int m_PosX{ 0 }, m_PosY{ 0 };
		int m_offsetX{ 0 }, m_offsetY{ 0 };
		int m_WheelX{ 0 }, m_WheelY{ 0 };
		bool m_bMouseMoving{ false };
	public:
		Mouse() = default;
		~Mouse() = default;

		void Update();
		void OnBtnPressed(int btn);
		void OnBtnReleased(int btn);

		const bool IsBtnPressed(int btn) const;
		const bool IsBtnJustPressed(int btn) const;
		const bool IsBtnJustReleased(int btn) const;

		inline void SetMouseWheelX(int wheel) { m_WheelX = wheel; }
		inline void SetMouseWheelY(int wheel) { m_WheelY = wheel; }
		inline void SetMouseOffset(int offsetX, int offsetY) { m_offsetX = offsetX; m_offsetY = offsetY; }
		inline void SetMouseMoving(bool moving) { m_bMouseMoving = moving;}

		inline const int GetMouseWheelX() const { return m_WheelX; }
		inline const int GetMouseWheelY() const { return m_WheelY; }
		const std::tuple<int, int>GetMouseOffset();
		const std::tuple<int, int> GetMouseScreenPosition();

		inline const bool IsMouseMoving() const { return m_bMouseMoving; }
	};
}