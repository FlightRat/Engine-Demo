#pragma once
#include<Core/ECS/Registry.h>
#include"IDisplay.h"

namespace ENGINE_EDITOR 
{
	class GameDisplay:public IDisplay
	{
	private:
		bool m_bPlayScene, m_bSceneLoaded;
	private:
		void LoadScene();
		void UnloadScene();
		void RenderGame();
	public:
		GameDisplay();
		~GameDisplay() = default;

		virtual void Draw() override;
		virtual void Update() override;
	};
}