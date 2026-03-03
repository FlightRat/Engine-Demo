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
#include<Core/ECS/Components/LightComponent.h>
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
#include<Physics/ContactListener.h>
#include<imgui.h>
#include<imgui_internal.h>
#include<backends/imgui_impl_sdl2.h>
#include<backends/imgui_impl_opengl3.h>
#include"editor/displays/IDisplay.h"
#include"editor/displays/GameDisplay.h"
#include"editor/displays/SceneDisplay.h"
#include"editor/displays/LogDisplay.h"
#include"editor/displays/AssetDisplay.h"
#include"editor/displays/MenuDisplay.h"
#include"editor/displays/SceneHierarchyDisplay.h"
#include"editor/utilities/editor_textures.h"
#include"editor/utilities/editor_framebuffers.h"
#include"editor/utilities/ComponentDrawer.h"
#include"editor/scene/SceneManager.h"
#include"editor/scene/SceneObject.h"

namespace ENGINE_EDITOR {
	Application::Application() :m_pWindow{ nullptr }, m_Event{}, m_bIsRunning{ true }
	{

	}

	bool Application::Initialize()
	{
		ENGINE_INIT_LOGS(false, true);

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

		if (!InitImGui())
		{
			ENGINE_ERROR("Failed to initialize ImGui!");
			return false;
		}

		// main registry
		auto& mainRegistry = MAIN_REGISTRY();
		if (!mainRegistry.Initialize())
		{
			ENGINE_ERROR("Failed to initialize the Main Registry!");
			return false;
		}
		// Render System
		auto renderSystem = std::make_shared<ENGINE_CORE::Systems::RenderSystem>();
		if (!renderSystem)
		{
			ENGINE_ERROR("Failed to create the render system!");
			return false;
		}
		if (!mainRegistry.AddToContext<std::shared_ptr< ENGINE_CORE::Systems::RenderSystem>>(renderSystem))
		{
			ENGINE_ERROR("Failed to add the render system to the registry context!");
			return false;
		}
		// editor framebuffer
		auto pEditorFramebuffer = std::make_shared<ENGINE_EDITOR::Editorframebuffers>();
		if (!pEditorFramebuffer)
		{
			ENGINE_ERROR("Failed to create the EditorFramebuffer");
			return false;
		}
		if (!mainRegistry.AddToContext<std::shared_ptr<ENGINE_EDITOR::Editorframebuffers>>(pEditorFramebuffer))
		{
			ENGINE_ERROR("Failed to add the EditorFramebuffer to the main registry context!");
			return false;
		}
		// game framebuffer
		auto gameFramebuffer = std::make_shared<ENGINE_RENDERING::Framebuffer>(600, 600, true);
		pEditorFramebuffer->mapFramebuffers.emplace(FramebufferType::GAME, gameFramebuffer);
		// scene framebuffer
		auto sceneFramebuffer = std::make_shared<ENGINE_RENDERING::Framebuffer>(600, 600, true);
		pEditorFramebuffer->mapFramebuffers.emplace(FramebufferType::SCENE, sceneFramebuffer);
		
		if (!CreateDisplays())
		{
			ENGINE_ERROR("Failed to create displays!");
			return false;
		}
		if (!LoadShaders())
		{
			ENGINE_ERROR("Failed to load the shaders!");
			return false;
		}
		if (!LoadEditorTextures())
		{
			ENGINE_ERROR("Failed to load the editor textures!");
			return false;
		}

		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::TransformComponent>();
		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::PhysicsComponent>();
		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::MeshFilter>();
		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::MeshRender>();
		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::Identification>();
		ComponentDrawer::RegisterUIComponent<ENGINE_CORE::ECS::LightComponent>();

		SCENE_MANAGER().AddScene("scene1");
		SCENE_MANAGER().AddScene("scene2");
		//SCENE_MANAGER().SetCurrentScene("scene1");

		return true;
	}

	bool Application::LoadShaders()
	{
		//auto& assetManager = m_pRegistry->GetContext<std::shared_ptr<ENGINE_RESOURCES::AssetManager>>();
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();

		// main shader
		if (!assetManager.AddShader("mainShader", "assets/shaders/mainShader.vert", "assets/shaders/mainShader.frag"))
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

		// bug shader
		if (!assetManager.AddShader("bugShader", "assets/shaders/bugShader.vert", "assets/shaders/bugShader.frag"))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// colliderShader
		if (!assetManager.AddShader("colliderShader", "assets/shaders/physicsDebugShader.vert", "assets/shaders/physicsDebugShader.frag"))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		return true;
	}

	bool Application::LoadEditorTextures()
	{
		auto& mainRegistry = MAIN_REGISTRY();
		auto& assetManager = mainRegistry.GetAssetManager();
		if (!assetManager.AddTextureFromMemory("play_button", play_button, sizeof(play_button) / sizeof(play_button[0])))
		{
			ENGINE_ERROR("Failed to load texture [play_button] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("stop_button", stop_button, sizeof(stop_button) / sizeof(stop_button[0])))
		{
			ENGINE_ERROR("Failed to load texture [stop_button] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("music_icon", music_icon, sizeof(music_icon) / sizeof(music_icon[0])))
		{
			ENGINE_ERROR("Failed to load texture [music_icon] from memory!");
			return false;
		}
		if (!assetManager.AddTextureFromMemory("scene_icon", scene_icon, sizeof(scene_icon) / sizeof(scene_icon[0])))
		{
			ENGINE_ERROR("Failed to load texture [scene_icon] from memory!");
			return false;
		}
		assetManager.GetTexture("play_button")->SetIsEditorTexture(true);
		assetManager.GetTexture("stop_button")->SetIsEditorTexture(true);
		assetManager.GetTexture("music_icon")->SetIsEditorTexture(true);
		assetManager.GetTexture("scene_icon")->SetIsEditorTexture(true);
		return true;
	}

	void Application::ProcessEvents()
	{
		auto pCurrentScene = SCENE_MANAGER().GetCurrentScene();
		//bool load = false;
		//if (pCurrentScene && pCurrentScene->CheckPlay())
		//{
		//	load = true;
		//	//auto& runtimeRegistry = pCurrentScene->GetRuntimeRegistry();
		//	//auto& camera = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
		//	//auto& physicsWorld = runtimeRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
		//}

		auto& inputManager = ENGINE_CORE::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		auto& mouse = inputManager.GetMouse();
		auto& engine = ENGINE_CORE::CoreEngineData::GetInstance();

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
				//if (m_Event.key.keysym.sym == SDLK_ESCAPE)
				//	m_bIsRunning = false;
				if (m_Event.key.keysym.sym == SDLK_0 && pCurrentScene)
				{
					engine.ToggleRenderCollisions();
					auto& runtimeRegistry = pCurrentScene->GetRegistry();
					//auto& camera = runtimeRegistry.GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
					auto& physicsWorld = runtimeRegistry.GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
					physicsWorld->setIsDebugRenderingEnabled(engine.RenderCollidersEnabled());
					auto view = runtimeRegistry.GetRegistry().view<ENGINE_CORE::ECS::PhysicsComponent>();
					for (auto [entity, physics] : view.each())
					{
						physics.SetDebug(engine.RenderCollidersEnabled());
					}
					physicsWorld->update(1.0f / 60.0f);
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
		ENGINE_CORE::CoreEngineData::GetInstance().UpdateDeltaTime();

		auto& mainRegistry = MAIN_REGISTRY();
		auto& displayHolder = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDITOR::DisplayHolder>>();
		for (const auto& pDisplay : displayHolder->displays)
			pDisplay->Update();

		auto& inputManager = ENGINE_CORE::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		auto& mouse = inputManager.GetMouse();
		keyboard.Update();
		mouse.Update();
	}

	void Application::Render()
	{
		BeginImGui();
		RenderImGui();
		EndImGui();
		SDL_GL_SwapWindow(m_pWindow->GetWindow().get());
	}

	void Application::CleanUp()
	{
		SDL_Quit();
	}

	bool Application::CreateDisplays()
	{
		auto& mainRegistry = MAIN_REGISTRY();

		auto pDisplayHolder = std::make_shared<ENGINE_EDITOR::DisplayHolder>();
		if (!pDisplayHolder)
		{
			ENGINE_ERROR("Failed to create the DisplayHolder");
			return false;
		}
		if (!mainRegistry.AddToContext<std::shared_ptr<ENGINE_EDITOR::DisplayHolder>>(pDisplayHolder))
		{
			ENGINE_ERROR("Failed to add the DisplayHolder to the registry context!");
			return false;
		}
	
		// game display
		auto pGameDisplay = std::make_unique<ENGINE_EDITOR::GameDisplay>();
		if (!pGameDisplay)
		{
			ENGINE_ERROR("Failed to create the GameDisplay");
			return false;
		}

		// scene display
		auto pSceneDisplay = std::make_unique< ENGINE_EDITOR::SceneDisplay>();
		if (!pSceneDisplay)
		{
			ENGINE_ERROR("Failed to create the SceneDisplay");
			return false;
		}

		// log display
		auto pLogDisplay = std::make_unique<ENGINE_EDITOR::LogDisplay>();
		if (!pLogDisplay)
		{
			ENGINE_ERROR("Failed to create the LogDisplay");
			return false;
		}

		// asset display
		auto pAssetDisplay = std::make_unique<ENGINE_EDITOR::AssetDisplay>();
		if (!pAssetDisplay)
		{
			ENGINE_ERROR("Failed to create the AssetDisplay");
			return false;
		}

		// MenuDisplay
		auto pMenuDisplay = std::make_unique<ENGINE_EDITOR::MenuDisplay>();
		if (!pMenuDisplay)
		{
			ENGINE_ERROR("Failed to create the MenuDisplay");
			return false;
		}

		// scene hierarchy display
		auto pSceneHierarchyDisplay = std::make_unique<ENGINE_EDITOR::SceneHierarchyDisplay>();
		if (!pSceneHierarchyDisplay)
		{
			ENGINE_ERROR("Failed to create the scene hierarchy display!");
			return false;
		}

		pDisplayHolder->displays.push_back(std::move(pGameDisplay));
		pDisplayHolder->displays.push_back(std::move(pSceneDisplay));
		pDisplayHolder->displays.push_back(std::move(pLogDisplay));
		pDisplayHolder->displays.push_back(std::move(pAssetDisplay));
		pDisplayHolder->displays.push_back(std::move(pMenuDisplay));
		pDisplayHolder->displays.push_back(std::move(pSceneHierarchyDisplay));
		
		return true;
	}

	bool Application::InitImGui()
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

	void Application::BeginImGui()
	{
		// start a new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}

	void Application::EndImGui()
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

	void Application::RenderImGui()
	{
		const auto dockSpaceId = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
		if (static auto firstTime = true; firstTime) [[unlikely]]
		{
			firstTime = false;
			ImGui::DockBuilderRemoveNode(dockSpaceId);
			ImGui::DockBuilderAddNode(dockSpaceId);
			auto centerNodeId = dockSpaceId;
			const auto leftNodeId = ImGui::DockBuilderSplitNode(centerNodeId, ImGuiDir_Left, 0.2f, nullptr, &centerNodeId);
			const auto downNodeId = ImGui::DockBuilderSplitNode(centerNodeId, ImGuiDir_Down, 0.2f, nullptr, &centerNodeId);
			const auto rightNodeId = ImGui::DockBuilderSplitNode(centerNodeId, ImGuiDir_Right, 0.25f, nullptr, &centerNodeId);
			//ImGui::DockBuilderDockWindow("Dear ImGui Demo", leftNodeId);
			ImGui::DockBuilderDockWindow("Scene Hierarchy", leftNodeId);
			ImGui::DockBuilderDockWindow("GO Details", rightNodeId);
			ImGui::DockBuilderDockWindow("Game", centerNodeId);
			ImGui::DockBuilderDockWindow("Scene", centerNodeId);
			ImGui::DockBuilderDockWindow("Asset", downNodeId);
			ImGui::DockBuilderDockWindow("Logs", downNodeId);
			ImGui::DockBuilderFinish(dockSpaceId);
		}

		auto& mainRegistry = MAIN_REGISTRY();
		auto& pDisplayHolder = mainRegistry.GetContext<std::shared_ptr<ENGINE_EDITOR::DisplayHolder>>();
		for (const auto& pDisplay : pDisplayHolder->displays)
		{
			pDisplay->Draw();
		}
		//ImGui::ShowDemoWindow();
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