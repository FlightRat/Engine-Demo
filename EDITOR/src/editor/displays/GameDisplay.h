#pragma once
#include <Core/ECS/Registry.h>
#include <glad/glad.h>
#include <memory>
namespace ENGINE_CORE::Systems { struct SSRViewState; }
#include"IDisplay.h"

namespace ENGINE_EDITOR 
{
	class GameDisplay:public IDisplay
	{
	private:
		std::unique_ptr<ENGINE_CORE::Systems::SSRViewState> m_Reflection;
		bool m_bPlayGame, m_bSceneLoaded;
	private:
		void PlayGame();
		void StopGame();
		GLuint RenderGame(int width, int height);
	protected:
		virtual void DrawToolbar() override;
	public:
		GameDisplay();
		~GameDisplay();

		virtual void Draw() override;
		virtual void Update() override;
	};
}