#pragma once
#include<glad/glad.h>
#include<Windowing/Window/Window.h>
#include<Core/ECS/Registry.h>

namespace EDITOR{
	class Application
	{
	private:
		std::unique_ptr<WINDOWING::Window> m_pWindow;
		std::unique_ptr<CORE::ECS::Registry> m_pRegistry;
		SDL_Event m_Event;
		bool m_bIsRunning;
		GLuint VAO, VBO, IBO;	//for test

	private:
		Application();
		bool Initialize();
		bool LoadShaders();
		void ProcessEvents();
		void Update();
		void Render();
		void CleanUp();
		
	public:
		static Application& GetInstance();
		~Application();

		void Run();
	};
}