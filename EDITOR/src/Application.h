#pragma once
#include<glad/glad.h>
#include<Windowing/Window/Window.h>
#include<Core/ECS/Registry.h>

namespace ENGINE_EDITOR::Automation { class AutomationRuntime; }
namespace ENGINE_EDITOR{
	class Application
	{
	private:
		std::unique_ptr<ENGINE_WINDOWING::Window> m_pWindow;
		SDL_Event m_Event;
		bool m_bIsRunning;
		std::unique_ptr<Automation::AutomationRuntime> m_pAutomation;

	private:
		Application();
		bool Initialize();
		bool LoadEditorShaders();
		bool LoadEditorTextures();
		bool LoadEditorMeshes();
		bool LoadEditorBuffers();
		void PrepareIBL();
		void ProcessEvents();
		void Update();
		void Render();
		void CleanUp();
		
		// ImgGui test
		bool CreateDisplays();
		bool InitImGui();
		void BeginImGui();
		void EndImGui();
		void RenderImGui();

	public:
		static Application& GetInstance();
		~Application();

		int Run(bool enableBridge = false, const std::string& instance = "default");
	};
}
