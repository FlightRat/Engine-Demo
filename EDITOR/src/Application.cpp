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
#include<Core/ECS/Components/PhysicsComponent.h>
#include<Core/ECS/Components/Identification.h>
#include<Core/Resources/AssetManager.h>
#include<Core/Systems/ScriptingSystem.h>
#include<Core/Systems/RenderSystem.h>
#include<Core/Systems/PhysicsSystem.h>
#include<Core/Scripting/InputManager.h>
#include<Windowing/Inputs/Keyboard.h>
#include<Sounds/MusicPlayer/MusicPlayer.h>
#include<Sounds/SoundFxPlayer/SoundFxPlayer.h>

const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

Uint64 now = SDL_GetPerformanceCounter();
Uint64 last = 0;
double deltaTime = 0;

namespace ENGINE_EDITOR {
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
		m_pWindow = std::make_unique<ENGINE_WINDOWING::Window>("Test", SCR_WIDTH, SCR_HEIGHT, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, true, SDL_WINDOW_OPENGL);
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
		m_pRegistry = std::make_unique<ENGINE_CORE::ECS::Registry>();

		// Asset Manager
		auto assetManager = std::make_shared<ENGINE_RESOURCES::AssetManager>();
		if (!assetManager)
		{
			ENGINE_ERROR("Failed to create the asset manager!");
			return false;
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr<ENGINE_RESOURCES::AssetManager>>(assetManager))
		{
			ENGINE_ERROR("Failed to add the asset manager to the registry context!");
			return false;
		}

		// Music Player
		auto musicPlayer = std::make_shared<ENGINE_SOUNDS::MusicPlayer>();
		if (!musicPlayer)
		{
			ENGINE_ERROR("Failed to create the music player!");
			return false;
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr<ENGINE_SOUNDS::MusicPlayer>>(musicPlayer))
		{
			ENGINE_ERROR("Failed to add the music player to the registry context!");
			return false;
		}

		// SoundFx Player
		auto soundFxPlayer = std::make_shared<ENGINE_SOUNDS::SoundFxPlayer>();
		if (!soundFxPlayer)
		{
			ENGINE_ERROR("Failed to create the soundFx player!");
			return false;
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr<ENGINE_SOUNDS::SoundFxPlayer>>(soundFxPlayer))
		{
			ENGINE_ERROR("Failed to add the soundFx player to the registry context!");
			return false;
		}

		// Physics Common
		std::shared_ptr<PhysicsCommon> physicsCommon = std::make_shared<PhysicsCommon>();
		if (!physicsCommon)
		{
			ENGINE_ERROR("Failed to create the physics common!");
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr<rp3d::PhysicsCommon>>(physicsCommon))
		{
			ENGINE_ERROR("Failed to add the physics common to the registry context!");
			return false;
		}

		// Physics World
		std::shared_ptr<PhysicsWorld> physicsWorld = ENGINE_PHYSICS::MakeSharedPhysicsWorld(physicsCommon);
		if(!physicsWorld)
		{
			ENGINE_ERROR("Failed to create the physics world!");
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr<rp3d::PhysicsWorld>>(physicsWorld))
		{
			ENGINE_ERROR("Failed to add the physics world to the registry context!");
			return false;
		}

		// Physics System
		auto physicsSystem = std::make_shared<ENGINE_CORE::Systems::PhysicsSystem>(*m_pRegistry);
		if (!physicsSystem)
		{
			ENGINE_ERROR("Failed to create the physics system!");
			return false;
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr< ENGINE_CORE::Systems::PhysicsSystem>>(physicsSystem))
		{
			ENGINE_ERROR("Failed to add the physics system to the registry context!");
			return false;
		}

		// Camera
		auto camera = std::make_shared<ENGINE_RENDERING::Camera3D>(glm::vec3(0.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -45.0f);		//平视相机
		//auto camera = std::make_shared<RENDERING::Camera3D>(glm::vec3(0.0f, 25.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 0.0f, -90.0f);		//俯视相机
		if (!m_pRegistry->AddToContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>(camera))
		{
			ENGINE_ERROR("Failed to add the camera to the registry context!");
			return false;
		}

		// TODO: Add shader to lua, like texture
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
		if (!m_pRegistry->AddToContext<std::shared_ptr<sol::state>>(lua))
		{
			ENGINE_ERROR("Failed to add the sol::state to the registry context!");
			return false;
		}

		// Script System
		auto scriptSystem = std::make_shared<ENGINE_CORE::Systems::ScriptingSystem>(*m_pRegistry);
		if (!scriptSystem)
		{
			ENGINE_ERROR("Failed to create the script system!");
			return false;
		}
		ENGINE_CORE::Systems::ScriptingSystem::RegisterLuaBindings(*lua, *m_pRegistry);
		ENGINE_CORE::Systems::ScriptingSystem::RegisterLuaFunctions(*lua);
		if (!scriptSystem->LoadMainScript(*lua))
		{
			ENGINE_ERROR("Failed to load the main lua script!");
			return false;
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr< ENGINE_CORE::Systems::ScriptingSystem>>(scriptSystem))
		{
			ENGINE_ERROR("Failed to add the script system to the registry context!");
			return false;
		}

		// Render System
		auto renderSystem = std::make_shared<ENGINE_CORE::Systems::RenderSystem>(*m_pRegistry);
		if (!renderSystem)
		{
			ENGINE_ERROR("Failed to create the render system!");
			return false;
		}
		if (!m_pRegistry->AddToContext<std::shared_ptr< ENGINE_CORE::Systems::RenderSystem>>(renderSystem))
		{
			ENGINE_ERROR("Failed to add the render system to the registry context!");
			return false;
		}

		// test dynamic cube
		//CORE::ECS::Entity cube{*m_pRegistry ,"cube", ""};
		//auto& cube_transform = cube.AddComponent<CORE::ECS::TransformComponent>(CORE::ECS::TransformComponent{
		//	.position = glm::vec3(0.0f, 5.0f, -5.0f),
		//	.scale = glm::vec3(0.5f),
		//	.rotation = glm::vec3(0.0f)
		//	});
		//auto& cube_mesh = cube.AddComponent<CORE::ECS::MeshComponent>(CORE::ECS::MeshComponent{
		//	.mesh = "cube",
		//	.shader = "colorShader",
		//	.color=glm::vec4(0.0f,1.0f,0.0f,1.0f),
		//	.texture = 0
		//	});
		//cube_mesh.load_mesh();
		//auto& cube_physics = cube.AddComponent<CORE::ECS::PhysicsComponent>(CORE::ECS::PhysicsComponent(
		//	CORE::ECS::PhysicsAttributes{
		//		.position = cube_transform.position,
		//		.rotation = cube_transform.rotation,
		//		.Type=BodyType::DYNAMIC,
		//		.Shape="box",
		//		.box_halfExtents={0.5,0.5,0.5}
		//	}));
		//cube_physics.Init(physicsCommon, physicsWorld);

		////test sphere
		//CORE::ECS::Entity sphere{ *m_pRegistry, "sphere", "" };
		//auto& sphere_transform = sphere.AddComponent<CORE::ECS::TransformComponent>(CORE::ECS::TransformComponent{
		//	.position = glm::vec3(0.0f, 5.0f, 5.0f),
		//	.scale = glm::vec3(0.5f),
		//	.rotation = glm::vec3(0.0f)
		//	});
		//auto& sphere_mesh = sphere.AddComponent<CORE::ECS::MeshComponent>(CORE::ECS::MeshComponent{
		//	.mesh = "sphere",
		//	.shader = "colorShader",
		//	.color = glm::vec4(0.0f,1.0f,0.0f,1.0f),
		//	.texture = 0
		//	});
		//sphere_mesh.load_mesh();
		//auto& sphere_physics = sphere.AddComponent<CORE::ECS::PhysicsComponent>(CORE::ECS::PhysicsComponent(
		//	CORE::ECS::PhysicsAttributes{
		//		.position = sphere_transform.position,
		//		.rotation = sphere_transform.rotation,
		//		.Type = BodyType::DYNAMIC,
		//		.Shape = "sphere",
		//		.sphere_radius = {0.5}
		//	}));
		//sphere_physics.Init(physicsCommon, physicsWorld);

		//// test static floor
		//CORE::ECS::Entity floor{ *m_pRegistry,"","" };
		//auto& floor_transform = floor.AddComponent<CORE::ECS::TransformComponent>(CORE::ECS::TransformComponent{
		//	.position = glm::vec3(0.0f, 0.0f, 0.0f),
		//	.scale = glm::vec3(1.0f),
		//	.rotation = glm::vec3(0.0f)
		//	});
		//auto& floor_mesh = floor.AddComponent<CORE::ECS::MeshComponent>(CORE::ECS::MeshComponent{
		//	.mesh = "plane",
		//	.shader = "colorShader",
		//	.color=glm::vec4(1.0f, 1.0f, 1.0f,1.0f),
		//	.texture = 0
		//	});
		//floor_mesh.load_mesh();
		//auto& floor_physics = floor.AddComponent<CORE::ECS::PhysicsComponent>(CORE::ECS::PhysicsComponent(
		//	CORE::ECS::PhysicsAttributes{
		//		.position = floor_transform.position,
		//		.rotation = floor_transform.rotation,
		//		.Type=BodyType::STATIC,
		//		.Shape="box",
		//		.box_halfExtents={10.0, 0.0005, 10.0},
		//	}));
		//floor_physics.Init(physicsCommon, physicsWorld);
	}

    bool Application::LoadShaders()
    {
		auto& assetManager = m_pRegistry->GetContext<std::shared_ptr<ENGINE_RESOURCES::AssetManager>>();

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

		// hud shader
		if (!assetManager->AddShader("hudShader", "assets/shaders/hudShader.vert", "assets/shaders/hudShader.frag"))
		{
			ENGINE_ERROR("Failed to create and add the shader!");
			return false;
		}

		return true;
    }

    void Application::ProcessEvents()
    {
		auto& camera = m_pRegistry->GetContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>();
		auto& inputManager = ENGINE_CORE::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		auto& mouse = inputManager.GetMouse();

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
					camera->ProcessKeyboard(ENGINE_RENDERING::FORWARD, deltaTime);
				else if (m_Event.key.keysym.sym == SDLK_LEFT)
					camera->ProcessKeyboard(ENGINE_RENDERING::LEFT, deltaTime);
				else if (m_Event.key.keysym.sym == SDLK_DOWN)
					camera->ProcessKeyboard(ENGINE_RENDERING::BACKWARD, deltaTime);
				else if (m_Event.key.keysym.sym == SDLK_RIGHT)
					camera->ProcessKeyboard(ENGINE_RENDERING::RIGHT, deltaTime);
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
				float xrel = static_cast<float>(m_Event.motion.xrel);
				float yrel = static_cast<float>(m_Event.motion.yrel);
				camera->ProcessMouseMovement(xrel, -yrel);
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				mouse.SetMouseWheelX(m_Event.wheel.x);
				mouse.SetMouseWheelY(m_Event.wheel.y);
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
		auto& scriptSystem = m_pRegistry->GetContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();
		scriptSystem->Update();

		auto& inputManager = ENGINE_CORE::InputManager::GetInstance();
		auto& keyboard = inputManager.GetKeyBoard();
		auto& mouse = inputManager.GetMouse();
		keyboard.Update();
		mouse.Update();

		auto& pw = m_pRegistry->GetContext<std::shared_ptr<rp3d::PhysicsWorld>>();
		auto& ps = m_pRegistry->GetContext<std::shared_ptr<ENGINE_CORE::Systems::PhysicsSystem>>();
		const decimal timeStep = 1.0f / 60.0f;
		pw->update(timeStep);
		ps->Update(m_pRegistry->GetRegistry());
    }

    void Application::Render()
    {

		last = now;
		now = SDL_GetPerformanceCounter();
		deltaTime = static_cast<double>(now - last) / SDL_GetPerformanceFrequency();

		glViewport(0, 0, m_pWindow->GetWidth(), m_pWindow->GetHeight());
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto& scriptSystem = m_pRegistry->GetContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>();
		scriptSystem->Render();

		auto& renderSystem = m_pRegistry->GetContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>();
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