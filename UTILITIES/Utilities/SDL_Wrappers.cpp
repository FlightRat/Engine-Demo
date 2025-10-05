#include "SDL_Wrappers.h"
#include<iostream>

void UTIL::SDL_Destoryer::operator()(SDL_Window* window) const
{
    SDL_DestroyWindow(window);
    ENGINE_LOG("SDL Window destoryed!");
}

void UTIL::SDL_Destoryer::operator()(SDL_GameController* controller) const
{
}

void UTIL::SDL_Destoryer::operator()(Mix_Chunk* chunk) const
{
    Mix_FreeChunk(chunk);
    ENGINE_LOG("Freed SDL Mix_Chunk!\n");
}

void UTIL::SDL_Destoryer::operator()(Mix_Music* music) const
{
    Mix_FreeMusic(music);
    ENGINE_LOG("Freed SDL Mix_Music!\n");
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
