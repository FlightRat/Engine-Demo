#pragma once
#include <Core/ECS/Registry.h>
#include <glad/glad.h>
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
		GLuint RenderGame(int width, int height);
	protected:
		virtual void DrawToolbar() override;
	public:
		GameDisplay();
		~GameDisplay() = default;

		virtual void Draw() override;
		virtual void Update() override;
	};
}