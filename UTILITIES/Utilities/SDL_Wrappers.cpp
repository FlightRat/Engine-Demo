#include "SDL_Wrappers.h"
#include<iostream>

void UTIL::SDL_Destoryer::operator()(SDL_Window* window) const
{
    SDL_DestroyWindow(window);
    std::cout << "SDL Window destoryed\n";
}

void UTIL::SDL_Destoryer::operator()(SDL_GameController* controller) const
{
}

void UTIL::SDL_Destoryer::operator()(SDL_Cursor* cursor) const
{
}

Controller make_shared_controller(SDL_GameController* controller)
{
    return Controller();
}

Cursor make_shared_cursor(SDL_Cursor* cursor)
{
    return Cursor();
}
