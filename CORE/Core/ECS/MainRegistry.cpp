#include "MainRegistry.h"
#include <Logger/Logger.h>
#include <SOIL/SOIL.h>
#include <../CORE/Core/Resources/AssetManager.h>
#include <../CORE/Core/Systems/ScriptingSystem.h>
#include <../CORE/Core/Systems/RenderSystem.h>
#include <../CORE/Core/Systems/PhysicsSystem.h>
#include <Sounds/MusicPlayer/MusicPlayer.h>
#include <Sounds/SoundFxPlayer/SoundFxPlayer.h>
#include <Rendering/Buffers/Framebuffer.h>
#include <Physics/ContactListener.h>

namespace ENGINE_CORE::ECS {
	Registry* MainRegistry::GetRegistry()
	{
		if (!m_pMainRegistry)
			m_pMainRegistry = std::make_unique<Registry>();

		return m_pMainRegistry.get();
	}

	MainRegistry& MainRegistry::GetInstance()
	{
		static MainRegistry instance{};
		return instance;
	}

	bool MainRegistry::Initialize()
	{
		m_pMainRegistry = std::make_unique<Registry>();
		assert(m_pMainRegistry && "Failed to initialize main registry!");

		// Camera
		auto camera = std::make_shared<ENGINE_RENDERING::Camera3D>(glm::vec3(0.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -45.0f);
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_RENDERING::Camera3D>>(std::move(camera));

		auto pAssetManager = std::make_shared<ENGINE_RESOURCES::AssetManager>();
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_RESOURCES::AssetManager>>(std::move(pAssetManager));

		auto pMusicPlayer = std::make_shared<ENGINE_SOUNDS::MusicPlayer>();
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_SOUNDS::MusicPlayer>>(std::move(pMusicPlayer));

		auto pSoundFxPlayer = std::make_shared<ENGINE_SOUNDS::SoundFxPlayer>();
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_SOUNDS::SoundFxPlayer>>(std::move(pSoundFxPlayer));

		auto pRenderSystem = std::make_shared<ENGINE_CORE::Systems::RenderSystem>(*m_pMainRegistry);
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_CORE::Systems::RenderSystem>>(std::move(pRenderSystem));

		auto pPhysicsSystem = std::make_shared<ENGINE_CORE::Systems::PhysicsSystem>(*m_pMainRegistry);
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_CORE::Systems::PhysicsSystem>>(std::move(pPhysicsSystem));

		auto pScriptingSystem = std::make_shared<ENGINE_CORE::Systems::ScriptingSystem>(*m_pMainRegistry);
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_CORE::Systems::ScriptingSystem>>(pScriptingSystem);

		// Physics Common
		std::shared_ptr<PhysicsCommon> physicsCommon = std::make_shared<PhysicsCommon>();
		m_pMainRegistry->AddToContext<std::shared_ptr<rp3d::PhysicsCommon>>(physicsCommon);

		// Physics World
		std::shared_ptr<PhysicsWorld> physicsWorld = ENGINE_PHYSICS::MakeSharedPhysicsWorld(physicsCommon);
		physicsWorld->getDebugRenderer().setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true);
		m_pMainRegistry->AddToContext<std::shared_ptr<rp3d::PhysicsWorld>>(physicsWorld);
		
		// Contact Listener
		auto contactListener = std::make_shared<ENGINE_PHYSICS::ContactListener>();
		physicsWorld->setEventListener(contactListener.get());
		m_pMainRegistry->AddToContext<std::shared_ptr< ENGINE_PHYSICS::ContactListener>>(contactListener);
		
		// test framebuffer
		auto framebuffer = std::make_shared<ENGINE_RENDERING::Framebuffer>(600, 600, true);
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_RENDERING::Framebuffer>>(std::move(framebuffer));

		m_bInitialized = true;

		// Lua script
		auto lua = std::make_shared<sol::state>();
		lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::table, sol::lib::io, sol::lib::string);
		m_pMainRegistry->AddToContext<std::shared_ptr<sol::state>>(lua);
		ENGINE_CORE::Systems::ScriptingSystem::RegisterLuaBindings(*lua, *m_pMainRegistry);
		ENGINE_CORE::Systems::ScriptingSystem::RegisterLuaFunctions(*lua);
		pScriptingSystem->LoadMainScript(*lua);

		return true;
	}

	ENGINE_RESOURCES::AssetManager& MainRegistry::GetAssetManager()
	{
		assert(m_bInitialized && "Main Registry must be initialized before use.");
		return *m_pMainRegistry->GetContext<std::shared_ptr<ENGINE_RESOURCES::AssetManager>>();
	}

	ENGINE_SOUNDS::MusicPlayer& MainRegistry::GetMusicPlayer()
	{
		assert(m_bInitialized && "Main Registry must be initialized before use.");
		return *m_pMainRegistry->GetContext<std::shared_ptr<ENGINE_SOUNDS::MusicPlayer>>();
	}

	ENGINE_SOUNDS::SoundFxPlayer& MainRegistry::GetSoundFxPlayer()
	{
		assert(m_bInitialized && "Main Registry must be initialized before use.");
		return *m_pMainRegistry->GetContext<std::shared_ptr<ENGINE_SOUNDS::SoundFxPlayer>>();
	}
}

