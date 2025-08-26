#include "Camera2D.h"

namespace RENDERING {
	Camera2D::Camera2D() :Camera2D(640, 480) {}

	Camera2D::Camera2D(int width, int height) :m_Width{ width }, m_Height{ height },
		m_Scale(1.f), m_Positon{ glm::vec2{0} },
		m_CameraMatrix{ 1.f }, m_OrthoProjection{ 1.f }, m_bNeedsUpdate{ true }
	{
		// Init ortho projection
		m_OrthoProjection = glm::ortho(0.f, static_cast<float>(m_Width), 0.f, static_cast<float>(m_Height), -1.f, 1.f);
		//m_OrthoProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	
		Update();
	}

	void Camera2D::Update()
	{
		if (!m_bNeedsUpdate)
			return;

		// Translate
		glm::vec3 translate{ 0.f, 0.f, 0.f };
		m_CameraMatrix = glm::translate(m_OrthoProjection, translate);

		// Scale
		glm::vec3 scale{ m_Scale, m_Scale, 0.f };
		m_CameraMatrix *= glm::scale(glm::mat4(1.f), scale);

		m_bNeedsUpdate = false;
	}
}

