#pragma once
#include<Core/ECS/Registry.h>
#include"IDisplay.h"

namespace ENGINE_RENDERING {
	class Camera3D;
}

namespace ENGINE_EDIOTR
{
	class SceneDisplay :public IDisplay
	{
	private:
		std::shared_ptr<ENGINE_RENDERING::Camera3D> m_pSceneCam;	//TODO: unique ptr
	private:
		void RenderScene();
		void LoadNewScene();
	public:
		SceneDisplay();
		~SceneDisplay() = default;

		virtual void Draw() override;
		virtual void Update() override;
	};
}