#include "Button.h"

void WINDOWING::INPUTS::Button::Update(bool bPressed)
{
	bJustPressed = !bIsPressed && bPressed;
	bJustReleased = bIsPressed && !bPressed;
	bIsPressed = bPressed;
}

void WINDOWING::INPUTS::Button::Reset()
{
	bJustPressed = false;
	bJustReleased = false;
}
