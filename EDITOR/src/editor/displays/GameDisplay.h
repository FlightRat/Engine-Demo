#pragma once
#include<Core/ECS/Registry.h>
#include"IDisplay.h"

namespace ENGINE_EDITOR 
{
	class GameDisplay:public IDisplay
	{
	private:
		bool m_bPlayGame, m_bSceneLoaded;
	private:
		void PlayGame();
		void StopGame();
		void RenderGame();
	public:
		GameDisplay();
		~GameDisplay() = default;

		virtual void Draw() override;
		virtual void Update() override;
	};
}