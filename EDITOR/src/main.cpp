#define SDL_MAIN_HANDLED 1
#define NOMINMAX
#include"Application.h"

#ifdef _WIN32
#include<Windows.h>
#endif

int main()
{
#ifndef NDEBUG
#ifdef _WIN32
	ShowWindow(GetConsoleWindow(), SW_SHOW);
#else
	//TODO: fow linux
#endif
#else
#ifdef _WIN32
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#else
	//TODO: fow linux
#endif
#endif

	auto& app = ENGINE_EDITOR::Application::GetInstance();
	app.Run();
	return 0;
}