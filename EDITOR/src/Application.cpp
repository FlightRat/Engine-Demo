#include "Application.h"
#include<SDL.h>
#include<glad/glad.h>
#include<iostream>
#include<SOIL/SOIL.h>
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include<Rendering/Essentials/ShaderLoader.h>
#include<Logger/Logger.h>
#include<Rendering/Essentials/TextureLoader.h>
#include<Rendering/Core/Camera3D.h>
#include<entt.hpp>
#include<Core/ECS/Entity.h>
#include<Core/ECS/Components/TransformComponent.h>
#include<Core/ECS/Components/MeshComponent.h>
#include<Core/ECS/Components/Identification.h>
#include<Core/Resources/AssetManager.h>
#include<Core/Systems/ScriptingSystem.h>
#include<Core/Systems/RenderSystem.h>
#include<Core/Scripting/InputManager.h>
#include<Windowing/Inputs/Keyboard.h>

const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

Uint64 now = SDL_GetPerformanceCounter();
Uint64 last = 0;
double deltaTime = 0;

namespace EDITOR {
    Application::Application():m_pWindow{nullptr},m_pRegistry{nullptr},m_Event{},m_bIsRunning{true}
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
		m_pWindow = std::make_unique<WINDOWING::Window>("Test", SCR_WIDTH, SCR_HEIGHT, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, true, SDL_WINDOW_OPENGL);
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
		SDL_SetRelativeMouseMode(SDL_TRUE);
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

		// ECS Registry
		m_pRegistry = std::make_unique<CORE::ECS::Registry>();

		// Asset Manager
		auto assetManager = std::make_shared<RESOURCES::AssetManager>();
		if (!assetManager)
		{
			ENGINE_ERROR("Failed to create the asset manager!");
			return false;
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr<RESOURCES::AssetManager>>(assetManager))
		{
			ENGINE_ERROR("Failed to add the asset manager to the registry context!");
			return false;
		}

		// Camera
		// auto camera = std::make_shared<RENDERING::Camera3D>(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);		//平视相机
		auto camera = std::make_shared<RENDERING::Camera3D>(glm::vec3(0.0f, 25.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 0.0f, -90.0f);		//俯视相机
		if (!m_pRegistry->AddToContext<std::shared_ptr<RENDERING::Camera3D>>(camera))
		{
			ENGINE_ERROR("Failed to add the camera to the registry context!");
			return false;
		}

		// textures
		if (!assetManager->AddTexture("mafuyu", "./assets/textures/mafuyu.png", false))
		{
			ENGINE_ERROR("Failed to create and add the texture!");
			return false;
		}

		// shaders
		if (!LoadShaders())
		{
			ENGINE_ERROR("Failed to load the shaders!");
			return false;
		}
		
		// Lua script
		auto lua = std::make_shared<sol::state>();
		if (!lua)
		{
			ENGINE_ERROR("Failed to create the lua state!");
			return false;
		}
		lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::table, sol::lib::io, sol::lib::string);
		if(!m_pRegistry->AddToContext<std::shared_ptr<sol::state>>(lua))
		{
			ENGINE_ERROR("Failed to add the sol::state to the registry context!");
			return false;
		}

		// Script System
		auto scriptSystem = std::make_shared<CORE::Systems::ScriptingSystem>(*m_pRegistry);
		if (!scriptSystem)
		{
			ENGINE_ERROR("Failed to create the script system!");
			return false;
		}
		CORE::Systems::ScriptingSystem::RegisterLuaBindings(*lua, *m_pRegistry);
		if (!scriptSystem->LoadMainScript(*lua))
		{
			ENGINE_ERROR("Failed to load the main lua script!");
			return false;
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr< CORE::Systems::ScriptingSystem>>(scriptSystem))
		{
			ENGINE_ERROR("Failed to add the script system to the registry context!");
			return false;
		}
		
		// Render System
		auto renderSystem = std::make_shared<CORE::Systems::RenderSystem>(*m_pRegistry);
		if (!renderSystem)
		{
			ENGINE_ERROR("Failed to create the render system!");
			return false;
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr< CORE::Systems::RenderSystem>>(renderSystem))
		{
			ENGINE_ERROR("Failed to add the render system to the registry context!");
			return false;
		}
	}

    bool Application::LoadShaders()
    {
		auto& assetManager = m_pRegistry->GetContext<std::shared_ptr<RESOURCES::AssetManager>>();

		// color shader
		if (!assetManager->AddShader("colorShader", "assets/shaders/colorShader.vert", "assets/shaders/colorShader.frag"))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		// tex shader
		if (!assetManager->AddShader("texShader", "assets/shaders/texShader.vert", "assets/shaders/texShader.frag"))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		return true;
    }

    void Application::ProcessEvents()
    {
		auto& camera = m_pRegistry->GetContext<std::shared_ptr<RENDERING::Camera3D>>();
		auto& inputManager = CORE::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();

		//process Events
		while (SDL_PollEvent(&m_Event))
		{
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
				else if (m_Event.key.keysym.sym == SDLK_UP)
					camera->ProcessKeyboard(RENDERING::FORWARD, deltaTime);
				else if (m_Event.key.keysym.sym == SDLK_LEFT)
					camera->ProcessKeyboard(RENDERING::LEFT, deltaTime);
				else if (m_Event.key.keysym.sym == SDLK_DOWN)
					camera->ProcessKeyboard(RENDERING::BACKWARD, deltaTime);
				else if (m_Event.key.keysym.sym == SDLK_RIGHT)
					camera->ProcessKeyboard(RENDERING::RIGHT, deltaTime);
				keyboard.OnKeyPressed(m_Event.key.keysym.sym);
				break;
			}
			case SDL_KEYUP:
			{
				keyboard.OnKeyReleased(m_Event.key.keysym.sym);
				break;
			}
			case SDL_MOUSEMOTION:
			{
				float xrel = static_cast<float>(m_Event.motion.xrel);
				float yrel = static_cast<float>(m_Event.motion.yrel);
				camera->ProcessMouseMovement(xrel, -yrel);
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				int xoffset = m_Event.wheel.x;
				int yoffset = m_Event.wheel.y;
				camera->ProcessMouseScroll(static_cast<float>(yoffset));
				break;
			}
			default:
				break;
			}
		}
    }

    void Application::Update()
    {
		// TODO: move the camera update here
		auto& scriptSystem = m_pRegistry->GetContext<std::shared_ptr<CORE::Systems::ScriptingSystem>>();
		scriptSystem->Update();

		auto& inputManager = CORE::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		keyboard.Update();
    }

    void Application::Render()
    {

		last = now;
		now = SDL_GetPerformanceCounter();
		deltaTime = static_cast<double>(now - last) / SDL_GetPerformanceFrequency();

		glViewport(0, 0, m_pWindow->GetWidth(), m_pWindow->GetHeight());
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto& scriptSystem = m_pRegistry->GetContext<std::shared_ptr<CORE::Systems::ScriptingSystem>>();
		scriptSystem->Render();

		auto& renderSystem = m_pRegistry->GetContext<std::shared_ptr<CORE::Systems::RenderSystem>>();
		renderSystem->Render();

		SDL_GL_SwapWindow(m_pWindow->GetWindow().get());
    }

    void Application::CleanUp()
    {
		SDL_Quit();
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