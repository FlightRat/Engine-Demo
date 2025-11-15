#pragma once
#include<Core/ECS/Registry.h>
#include"IDisplay.h"

namespace ENGINE_EDIOTR 
{
	class SceneDisplay:public IDisplay
	{
	private:
		ENGINE_CORE::ECS::Registry& m_Registry;
		bool m_bPlayScene, m_bSceneLoaded;
	private:
		void LoadScene();
		void UnloadScene();
		void RenderScene();
	public:
		SceneDisplay(ENGINE_CORE::ECS::Registry& registry);
		~SceneDisplay() = default;

		virtual void Draw() override;
		virtual void Update() override;
	};
}