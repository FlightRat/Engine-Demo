#pragma once
#include "Registry.h"

#define MAIN_REGISTRY() ENGINE_CORE::ECS::MainRegistry::GetInstance()

namespace ENGINE_CORE::RESOURCES { class AssetManager; }
namespace ENGINE_SOUNDS { class MusicPlayer; class SoundFxPlayer; }

namespace ENGINE_CORE::ECS {
	class MainRegistry
	{
	private:
		std::unique_ptr<Registry> m_pMainRegistry{ nullptr };
		bool m_bInitialized{ false };

		MainRegistry() = default;
		~MainRegistry() = default;
		MainRegistry(const MainRegistry&) = delete;
		MainRegistry& operator =(const MainRegistry&) = delete;
	public:
		static MainRegistry& GetInstance();
		bool Initialize();

		template <typename TContext>
		TContext AddToContext(TContext context)
		{
			return m_pMainRegistry->AddToContext<TContext>(context);
		}

		template <typename TContext>
		TContext& GetContext()
		{
			return m_pMainRegistry->GetContext<TContext>();
		}

		ENGINE_CORE::RESOURCES::AssetManager& GetAssetManager();
		ENGINE_SOUNDS::MusicPlayer& GetMusicPlayer();
		ENGINE_SOUNDS::SoundFxPlayer& GetSoundFxPlayer();
	};
}