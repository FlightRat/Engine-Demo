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

auto camera = std::make_shared<RENDERING::Camera3D>(glm::vec3(0.0f, 0.0f, 3.0f));

const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

Uint64 now = SDL_GetPerformanceCounter();
Uint64 last = 0;
double deltaTime = 0;

void mouse_callback(double xposIn, double yposIn);
void scroll_callback(double xoffset, double yoffset);

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
		//auto camera = std::make_shared<RENDERING::Camera3D>(glm::vec3(0.0f, 0.0f, 3.0f));
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

		// cube entity
		CORE::ECS::Entity entity1{ *m_pRegistry,"cube","Test" };
		auto& transform_1 = entity1.AddComponent<CORE::ECS::TransformComponent>(CORE::ECS::TransformComponent{
			.position = glm::vec3{0.f, 0.f, 0.f},
			.scale = glm::vec3{0.5f},
			});
		auto& mesh_1 = entity1.AddComponent<CORE::ECS::MeshComponent>(CORE::ECS::MeshComponent{"cube"});
		auto& id_1 = entity1.GetComponent<CORE::ECS::Identification>();
		// plane entity
		CORE::ECS::Entity entity2{ *m_pRegistry,"plane","Test" };
		auto& transform_2 = entity2.AddComponent<CORE::ECS::TransformComponent>(CORE::ECS::TransformComponent{
			.position = glm::vec3{0.f, 0.f, 0.f},
			.scale = glm::vec3{1.0f},
			});
		auto& mesh_2 = entity2.AddComponent<CORE::ECS::MeshComponent>(CORE::ECS::MeshComponent{ "plane" });
		auto& id_2 = entity2.GetComponent<CORE::ECS::Identification>();
		
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
				else if (m_Event.key.keysym.sym == SDLK_w)
					camera->ProcessKeyboard(RENDERING::FORWARD, deltaTime);
				else if (m_Event.key.keysym.sym == SDLK_a)
					camera->ProcessKeyboard(RENDERING::LEFT, deltaTime);
				else if (m_Event.key.keysym.sym == SDLK_s)
					camera->ProcessKeyboard(RENDERING::BACKWARD, deltaTime);
				else if (m_Event.key.keysym.sym == SDLK_d)
					camera->ProcessKeyboard(RENDERING::RIGHT, deltaTime);
				break;
			}
			case SDL_MOUSEMOTION:
			{
				float xrel = static_cast<float>(m_Event.motion.xrel);
				float yrel = static_cast<float>(m_Event.motion.yrel);
				// 直接将相对偏移量传递给摄像机，不再需要 mouse_callback 函数
				camera->ProcessMouseMovement(xrel, -yrel); // 注意：y轴方向可能需要反转
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				int xoffset = m_Event.wheel.x;
				int yoffset = m_Event.wheel.y;
				scroll_callback(xoffset, yoffset);
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

void mouse_callback(double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;
	camera->ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(double xoffset, double yoffset)
{
	camera->ProcessMouseScroll(static_cast<float>(yoffset));
}
