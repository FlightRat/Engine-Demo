#pragma once

namespace ENGINE_WINDOWING::INPUTS {
	struct Button
	{
		bool bIsPressed{ false }, bJustPressed{ false }, bJustReleased{ false };
		void Update(bool bPressed);
		void Reset();
	};
}