#pragma once

namespace WINDOWING::INPUTS {
	struct Button
	{
		bool bIsPressed{ false }, bJustPressed{ false }, bJustReleased{ false };
		void Update(bool bPressed);
		void Reset();
	};
}