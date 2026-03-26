#include "MainRegistry.h"
#include <Logger/Logger.h>
#include <../CORE/Core/Buffers/BufferManager.h>
#include <../CORE/Core/Resources/AssetManager.h>
#include <SOUNDS/MusicPlayer/MusicPlayer.h>
#include <Sounds/SoundFxPlayer/SoundFxPlayer.h>

namespace ENGINE_CORE::ECS {
	MainRegistry& MainRegistry::GetInstance()
	{
		static MainRegistry instance{};
		return instance;
	}
	bool MainRegistry::Initialize()
	{
		m_pMainRegistry = std::make_unique<Registry>();
		assert(m_pMainRegistry && "Failed to initialize main registry!");

		auto pAssetManager = std::make_shared<ENGINE_CORE::RESOURCES::AssetManager>();
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_CORE::RESOURCES::AssetManager>>(std::move(pAssetManager));

		auto pMusicPlayer = std::make_shared<ENGINE_SOUNDS::MusicPlayer>();
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_SOUNDS::MusicPlayer>>(std::move(pMusicPlayer));

		auto pSoundFxPlayer = std::make_shared<ENGINE_SOUNDS::SoundFxPlayer>();
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_SOUNDS::SoundFxPlayer>>(std::move(pSoundFxPlayer));

		auto pBufferManager = std::make_shared<ENGINE_CORE::BUFFERS::BufferManager>();
		m_pMainRegistry->AddToContext<std::shared_ptr<ENGINE_CORE::BUFFERS::BufferManager>>(std::move(pBufferManager));

		m_bInitialized = true;
		return true;
	}

	ENGINE_CORE::BUFFERS::BufferManager& MainRegistry::GetBufferManager()
	{
		assert(m_bInitialized && "Main Registry must be initialized before use.");
		return *m_pMainRegistry->GetContext<std::shared_ptr<ENGINE_CORE::BUFFERS::BufferManager>>();
	}

	ENGINE_CORE::RESOURCES::AssetManager& MainRegistry::GetAssetManager()
	{
		assert(m_bInitialized && "Main Registry must be initialized before use.");
		return *m_pMainRegistry->GetContext<std::shared_ptr<ENGINE_CORE::RESOURCES::AssetManager>>();
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

