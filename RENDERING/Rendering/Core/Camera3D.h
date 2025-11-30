#pragma once
#include <glad/glad.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>

namespace ENGINE_RENDERING {
	// Default camera values
	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float SPEED = 10.0f;
	const float SENSITIVITY = 0.1f;
	const float ZOOM = 43.0f;

	enum Camera_Movement {
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT
	};

	class Camera3D
	{
	public:
		int Width = {600};
		int Height = { 600 };
		// camera Attributes
		glm::vec3 Position;
		glm::vec3 Front;
		glm::vec3 Up;
		glm::vec3 Right;
		glm::vec3 WorldUp;
		// euler Angles
		float Yaw;
		float Pitch;
		// camera options
		float MovementSpeed;
		float MouseSensitivity;
		float Zoom;

		Camera3D(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
		Camera3D(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

		void Reset();

		void LookAt(const glm::vec3& target);

		inline int GetWidth() const { return Width; }
		inline int GetHeight() const { return Height; }
		inline void SetWidth(int width) { Width = width; }
		inline void SetHeight(int height) { Height = height; }

		inline glm::vec3 GetPosition() const { return Position; }
		inline float GetZoom() const { return Zoom; }
		inline void SetPosition(glm::vec3 newPosition) { Position = newPosition; }
		inline void SetZoom(float newZoom) { Zoom = newZoom; }

		glm::mat4 GetViewMatrix();

		void ProcessKeyboard(Camera_Movement direction);
		void ProcessKeyboard(Camera_Movement direction,double dt);
		void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);
		void ProcessMouseScroll(float yoffset);

	private:
		void updateCameraVectors();
	};
}