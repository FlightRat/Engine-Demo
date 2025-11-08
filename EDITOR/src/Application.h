#pragma once
#include<glad/glad.h>
#include<Windowing/Window/Window.h>
#include<Core/ECS/Registry.h>

namespace ENGINE_EDITOR{
	class Application
	{
	private:
		std::unique_ptr<ENGINE_WINDOWING::Window> m_pWindow;
		SDL_Event m_Event;
		bool m_bIsRunning;

	private:
		Application();
		bool Initialize();
		bool LoadShaders();
		void ProcessEvents();
		void Update();
		void Render();
		void CleanUp();
		
		// ImgGui test
		bool CreateDisplays();
		bool ImGui_Init();
		void ImGui_Begin();
		void ImGui_Render();
		void ImGui_End();

	public:
		static Application& GetInstance();
		~Application();

		void Run();
	};
}