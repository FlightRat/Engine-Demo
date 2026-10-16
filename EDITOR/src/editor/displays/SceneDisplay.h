#pragma once
#include <Core/ECS/Registry.h>
#include <glad/glad.h>
#include <memory>
namespace ENGINE_CORE::Systems { struct SSRViewState; }
#include"IDisplay.h"

namespace ENGINE_RENDERING {
	class Camera3D;
}

namespace ENGINE_EDITOR
{
	class SceneDisplay :public IDisplay
	{
	private:
		std::unique_ptr<ENGINE_CORE::Systems::SSRViewState> m_Reflection;
		std::shared_ptr<ENGINE_RENDERING::Camera3D> m_pSceneCam;	//TODO: unique ptr
	private:
		void LoadScnne();
		GLuint RenderScene(int width, int height);
		void LoadNewScene();
	protected:
		virtual void DrawToolbar() override;
	public:
		SceneDisplay();
		~SceneDisplay();

		virtual void Draw() override;
		virtual void Update() override;

		void ControlCam();
	};
}