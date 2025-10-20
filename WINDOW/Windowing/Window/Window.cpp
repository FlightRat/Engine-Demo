#include "Window.h"
#include<iostream>

namespace ENGINE_WINDOWING
{
	void Window::CreateNewWindow(Uint32 flags)
	{
		m_pWindow = WindowPtr(SDL_CreateWindow(
			m_sTitle.c_str(),
			m_XPos,m_YPos,m_Width,m_Height,flags
		));

		//check
		if (!m_pWindow)
		{
			std::string error = SDL_GetError();
			ENGINE_ERROR("Failed to create the window:{0}", error);
		}
	}

	Window::Window(const std::string title, int width, int height, int x_pos, int y_pos, bool v_sync, Uint32 flags)
		:m_pWindow{nullptr},m_GLContext{},m_sTitle{title},
		m_Width{width}, m_Height{height}, m_XPos{x_pos}, m_YPos{y_pos},m_WindowFlags{flags}
	{
		CreateNewWindow(flags);

		//check v-sync
		if (v_sync)
		{
			if (!SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1"))
				ENGINE_ERROR("Failed to enable v - sync!");
		}
		ENGINE_LOG("Window Created Successfully!");
	}

	Window::~Window()
	{
	}

	void Window::SetWindowName(const std::string& name)
	{
		m_sTitle = name;
		SDL_SetWindowTitle(m_pWindow.get(), name.c_str());
	}
}


