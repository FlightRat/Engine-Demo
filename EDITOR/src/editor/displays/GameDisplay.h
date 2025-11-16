#pragma once
#include<Core/ECS/Registry.h>
#include"IDisplay.h"

namespace ENGINE_EDIOTR 
{
	class GameDisplay:public IDisplay
	{
	private:
		ENGINE_CORE::ECS::Registry& m_Registry;
		bool m_bPlayScene, m_bSceneLoaded;
	private:
		void LoadScene();
		void UnloadScene();
		void RenderGame();
	public:
		GameDisplay(ENGINE_CORE::ECS::Registry& registry);
		~GameDisplay() = default;

		virtual void Draw() override;
		virtual void Update() override;
	};
}