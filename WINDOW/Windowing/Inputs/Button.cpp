#include "Button.h"

void ENGINE_WINDOWING::INPUTS::Button::Update(bool bPressed)
{
	bJustPressed = !bIsPressed && bPressed;
	bJustReleased = bIsPressed && !bPressed;
	bIsPressed = bPressed;
}

void ENGINE_WINDOWING::INPUTS::Button::Reset()
{
	bJustPressed = false;
	bJustReleased = false;
}
