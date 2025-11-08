#include "Application.h"
#include<SDL.h>
#include<SDL_opengl.h>
#include<glad/glad.h>
#include<iostream>
#include<SOIL/SOIL.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<Logger/Logger.h>
#include<Rendering/Essentials/ShaderLoader.h>
#include<Rendering/Essentials/TextureLoader.h>
#include<Rendering/Core/Camera3D.h>
#include<Rendering/Buffers/Framebuffer.h>
#include<entt.hpp>
#include<Core/ECS/Entity.h>
#include<Core/ECS/MainRegistry.h>
#include<Core/ECS/Components/TransformComponent.h>
#include<Core/ECS/Components/PhysicsComponent.h>
#include<Core/ECS/Components/Identification.h>
#include<Core/Resources/AssetManager.h>
#include<Core/Systems/ScriptingSystem.h>
#include<Core/Systems/RenderSystem.h>
#include<Core/Systems/PhysicsSystem.h>
#include<Core/Scripting/InputManager.h>
#include<Core/CoreUtilities/CoreEngineData.h>
#include<Windowing/Inputs/Keyboard.h>
#include<Sounds/MusicPlayer/MusicPlayer.h>
#include<Sounds/SoundFxPlayer/SoundFxPlayer.h>
#include<Physics/ContactListener.h>
#include<imgui.h>
#include<imgui_internal.h>
#include<backends/imgui_impl_sdl2.h>
#include<backends/imgui_impl_opengl3.h>
#include"editor/displays/IDisplay.h"
#include"editor/displays/SceneDisplay.h"
#include"editor/displays/LogDisplay.h"

double accumulator = 0; //TODO:where should it be???

namespace ENGINE_EDITOR {
    Application::Application():m_pWindow{nullptr}, m_Event{},m_bIsRunning{true}
	{

	}

	bool Application::Initialize()
	{
		ENGINE_INIT_LOGS(true, true);

		// Init SDL
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		{
			std::string error = SDL_GetError();
			ENGINE_ERROR("Failed to initialize SDL: {0}", error);
			return false;
		}

		// Init OpenGL
		if (SDL_GL_LoadLibrary(NULL) != 0)
		{
			std::string error = SDL_GetError();
			ENGINE_ERROR("Failed to initialize OpenGL: {0}", error);
			return false;
		}

		// Set the OpenGL attributes
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		// Set per channel bits
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

		// Create the window
		SDL_DisplayMode displayMode;
		SDL_GetCurrentDisplayMode(0, &displayMode);
		m_pWindow = std::make_unique<ENGINE_WINDOWING::Window>(
			"Test",
			displayMode.w,
			displayMode.h,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, true, 
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_MAXIMIZED);
		if (!m_pWindow->GetWindow())
		{
			ENGINE_ERROR("Failed to create the window!");
			return false;
		}

		// Create OpenGL context
		m_pWindow->SetGLContext(SDL_GL_CreateContext(m_pWindow->GetWindow().get()));
		if (!m_pWindow->GetGLContext())
		{
			std::string error = SDL_GetError();
			ENGINE_ERROR("Failed to create OpenGL context: {0}", error);
			return false;
		}

		SDL_GL_MakeCurrent(m_pWindow->GetWindow().get(), m_pWindow->GetGLContext());
		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_GL_SetSwapInterval(1);

		//Initialze Glad
		if (gladLoadGLLoader(SDL_GL_GetProcAddress) == 0)
		{
			ENGINE_ERROR("Failed to loadGL --> GLAD");
			return false;
		}

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (!ImGui_Init())
		{
			ENGINE_ERROR("Failed to initialize ImGui!");
			return false;
		}

		auto& mainRegistry = MAIN_REGISTRY();
		if (!mainRegistry.Initialize())
		{
			ENGINE_ERROR("Failed to initialize the Main Registry!");
			return false;
		}

		// shaders TODO: Add shader to lua, like texture
		if (!LoadShaders())
		{
			ENGINE_ERROR("Failed to load the shaders!");
			return false;
		}

		CreateDisplays();

		return true;
	}

	bool Application::LoadShaders()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();
		// color shader
		if (!assetManager.AddShader("colorShader", "assets/shaders/colorShader.vert", "assets/shaders/colorShader.frag"))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// tex shader
		if (!assetManager.AddShader("texShader", "assets/shaders/texShader.vert", "assets/shaders/texShader.frag"))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// hud shader
		if (!assetManager.AddShader("hudShader", "assets/shaders/hudShader.vert", "assets/shaders/hudShader.frag"))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// physicsDebugShader
		if (!assetManager.AddShader("debugShader", "assets/shaders/physicsDebugShader.vert", "assets/shaders/physicsDebugShader.frag"))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		return true;
	}

	void Application::ProcessEvents()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& camera = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
		auto& inputManager = ENGINE_CORE::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		auto& mouse = inputManager.GetMouse();

		auto& engine = ENGINE_CORE::CoreEngineData::GetInstance();
		auto& physicsWorld = mainRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();

		//process Events
		while (SDL_PollEvent(&m_Event))
		{
			ImGui_ImplSDL2_ProcessEvent(&m_Event);
			switch (m_Event.type)
			{
			case SDL_QUIT:
			{
				m_bIsRunning = false;
				break;
			}
			case SDL_KEYDOWN:
			{
				if (m_Event.key.keysym.sym == SDLK_ESCAPE)
					m_bIsRunning = false;
				if (m_Event.key.keysym.sym == SDLK_0)
				{
					engine.ToggleRenderCollisions();
					physicsWorld->setIsDebugRenderingEnabled(engine.RenderCollidersEnabled());
					auto view = mainRegistry.GetRegistry()->GetRegistry().view<ENGINE_CORE::ECS::PhysicsComponent>();
					for (auto [entity, physics] : view.each())
					{
						physics.SetDebug(engine.RenderCollidersEnabled());
					}
				}
				keyboard.OnKeyPressed(m_Event.key.keysym.sym);
				break;
			}
			case SDL_KEYUP:
			{
				keyboard.OnKeyReleased(m_Event.key.keysym.sym);
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				mouse.OnBtnPressed(m_Event.button.button);
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				mouse.OnBtnReleased(m_Event.button.button);
				break;
			}
			case SDL_MOUSEMOTION:
			{
				mouse.SetMouseMoving(true);
				mouse.SetMouseOffset(m_Event.motion.xrel, m_Event.motion.yrel);
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				mouse.SetMouseWheelX(m_Event.wheel.x);
				mouse.SetMouseWheelY(m_Event.wheel.y);

				break;
			}
			case SDL_WINDOWEVENT: // MODIFICATION: ¼àÌý´°¿ÚÊÂ¼þ
			{
				if (m_Event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				{
					int newWidth = m_Event.window.data1;
					int newHeight = m_Event.window.data2;
					m_pWindow->SetWidth(newWidth);
					m_pWindow->SetHeight(newHeight);
					//ENGINE_CORE::CoreEngineData::GetInstance().SetWindowWidth(newWidth);
					//ENGINE_CORE::CoreEngineData::GetInstance().SetWindowHeight(newHeight);
				}
				break;
			}
			default:
				break;
			}
		}
	}

	void Application::Update()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		ENGINE_CORE::CoreEngineData::GetInstance().UpdateDeltaTime();
		double deltaTime = ENGINE_CORE::CoreEngineData::GetInstance().GetDeltaTime();
		// maybe should move deltatime into lua instead, by the way, the physics velocity need delta too
		const double MAX_DELTA_TIME = 0.25;
		if (deltaTime > MAX_DELTA_TIME)
		{
			deltaTime = MAX_DELTA_TIME;
		}
		
		const decimal timeStep = ENGINE_CORE::CoreEngineData::GetInstance().GetPhysicsTimeStep();
		accumulator += deltaTime;

		// TODO: move the camera update here
		auto& scriptSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();
		scriptSystem->Update();

		auto& inputManager = ENGINE_CORE::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		auto& mouse = inputManager.GetMouse();
		keyboard.Update();
		mouse.Update();

		auto& physicsWorld = mainRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
		auto& physicsSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::PhysicsSystem>>();
		while (accumulator >= timeStep) {
			physicsWorld->update(timeStep);
			accumulator -= timeStep;
		}
		decimal factor = accumulator / timeStep;
		physicsSystem->Update(mainRegistry.GetRegistry()->GetRegistry(), factor);
	}

	void Application::Render()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		//TODO: add w&h param for camera, and set them here, then pass the camera into render func
		auto& framebuffer = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Framebuffer>>();
		framebuffer->Bind();

		glViewport(0, 0, framebuffer->Width(), framebuffer->Height());
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// camera
		auto& camera = mainRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
		camera->SetWidth(framebuffer->Width());
		camera->SetHeight(framebuffer->Height());

		auto& scriptSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();
		scriptSystem->Render();
		auto& renderSystem = mainRegistry.GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
		renderSystem->Render(camera);

		framebuffer->Unbind();

		ImGui_Begin();
		ImGui_Render();
		ImGui_End();

		framebuffer->CheckResize();

		SDL_GL_SwapWindow(m_pWindow->GetWindow().get());
	}

	void Application::CleanUp()
	{
		SDL_Quit();
	}

	bool Application::CreateDisplays()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto pDisplayHolder = std::make_shared<ENGINE_EDIOTR::DisplayHolder>();
		if (!pDisplayHolder)
		{
			ENGINE_ERROR("Failed to create the DisplayHolder");
			return false;
		}
		if (!mainRegistry.AddToContext<std::shared_ptr<ENGINE_EDIOTR::DisplayHolder>>(pDisplayHolder))
		{
			ENGINE_ERROR("Failed to add the DisplayHolder to the registry context!");
			return false;
		}
	
		// scene display
		auto pSceneDisplay = std::make_unique<ENGINE_EDIOTR::SceneDisplay>(*mainRegistry.GetRegistry());
		if (!pSceneDisplay)
		{
			ENGINE_ERROR("Failed to create the SceneDisplay");
			return false;
		}

		// log display
		auto pLogDisplay = std::make_unique<ENGINE_EDIOTR::LogDisplay>();
		if (!pLogDisplay)
		{
			ENGINE_ERROR("Failed to create the LogDisplay");
			return false;
		}
		
		pDisplayHolder->displays.push_back(std::move(pSceneDisplay));
		pDisplayHolder->displays.push_back(std::move(pLogDisplay));

		return true;
	}

	bool Application::ImGui_Init()
	{
		const char* glslVersion = "#version 450";
		IMGUI_CHECKVERSION();
		if (!ImGui::CreateContext())
		{
			ENGINE_ERROR("Failed to create ImGui Context");
			return false;
		}

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		if (!ImGui_ImplSDL2_InitForOpenGL(m_pWindow->GetWindow().get(), m_pWindow->GetGLContext()))
		{
			ENGINE_ERROR("Failed to initialize ImGui SDL2 for OpenGL!");
			return false;
		}
		if (!ImGui_ImplOpenGL3_Init(glslVersion))
		{
			ENGINE_ERROR("Failed to initialize ImGui OpenGL3!");
			return false;
		}

		return true;
	}

	void Application::ImGui_Begin()
	{
		// start a new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}

	void Application::ImGui_End()
	{
		ImGui::Render(); // generate data for imgui to render
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			SDL_GLContext backupContext = SDL_GL_GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			SDL_GL_MakeCurrent(m_pWindow->GetWindow().get(), backupContext);
		}
	}

	void Application::ImGui_Render()
	{
		auto& mainRegistry = MAIN_REGISTRY();

		const auto dockSpaceId = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
		if (static auto firstTime = true; firstTime) [[unlikely]]
		{
			firstTime = false;
			ImGui::DockBuilderRemoveNode(dockSpaceId);
			ImGui::DockBuilderAddNode(dockSpaceId);
			auto centerNodeId = dockSpaceId;
			const auto leftNodeId = ImGui::DockBuilderSplitNode(centerNodeId, ImGuiDir_Left, 0.2f, nullptr, &centerNodeId);
			const auto logNodeId = ImGui::DockBuilderSplitNode(centerNodeId, ImGuiDir_Down, 0.2f, nullptr, &centerNodeId);
			ImGui::DockBuilderDockWindow("Dear ImGui Demo", leftNodeId);
			ImGui::DockBuilderDockWindow("Scene", centerNodeId);
			ImGui::DockBuilderDockWindow("Logs", logNodeId);
			ImGui::DockBuilderFinish(dockSpaceId);
		}
		auto& pDisplayHolder = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDIOTR::DisplayHolder>>();
		for (const auto& pDisplay : pDisplayHolder->displays)
		{
			pDisplay->Draw();
		}
		ImGui::ShowDemoWindow();
	}

	Application& Application::GetInstance()
	{
		static Application app{};
		return app;
	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		if (!Initialize())
		{
			ENGINE_ERROR("Initialization failed!");
			return;
		}

		while (m_bIsRunning)
		{
			ProcessEvents();
			Update();
			Render();
		}
		CleanUp();
	}
}