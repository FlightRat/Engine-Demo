#pragma once
#include <glm/glm.hpp>

namespace ENGINE_RENDERING { class Camera3D; }
namespace ENGINE_CORE::ECS { class Registry; }

namespace ENGINE_EDITOR {
	class AbstractTool
	{
	private:
		glm::vec2 m_MouseScreenCoords, m_MouseWorldCoords;
		glm::vec2 m_GUICursorCoords, m_GUIRelativeCoords;
		glm::vec2 m_WindowPos, m_WindowSize;
		bool m_bActivated, m_bOutOfBounds;

		void UpdateMouseWorldCoords();
		void CheckOutOfBounds();

	protected:
		enum class EMouseButton
		{
			UNKNOWN = 0,
			LEFT,
			MIDDLE,
			RIGHT,
			LAST
		};
		ENGINE_CORE::ECS::Registry* m_pRegistry{ nullptr };
		ENGINE_RENDERING::Camera3D* m_pCamera{ nullptr };

		bool MouseBtnJustPressed(EMouseButton eButton);
		bool MouseBtnJustReleased(EMouseButton eButton);
		bool MouseBtnPressed(EMouseButton eButton);
		bool MouseMoving();
		virtual void ExamineMousePosition() = 0;
		inline void SetMouseWorldCoords(const glm::vec2& newCoords) { m_MouseWorldCoords = newCoords; }

	public:
		AbstractTool();
		virtual ~AbstractTool() = default;
		virtual void Update();
		bool SetupTool(ENGINE_CORE::ECS::Registry* pRegistry, ENGINE_RENDERING::Camera3D* pCamera);

		inline void SetRelativeCoords(const glm::vec2& relativeCoords) { m_GUIRelativeCoords = relativeCoords; }
		inline void SetCursorCoords(const glm::vec2& cursorCoords) { m_GUICursorCoords = cursorCoords; }
		inline void SetWindowPos(const glm::vec2& windowPos) { m_WindowPos = windowPos; }
		inline void SetWindowSize(const glm::vec2& windowSize) { m_WindowSize = windowSize; }

		inline const glm::vec2& GetMouseScreenCoords() const { return m_MouseScreenCoords; }
		inline const glm::vec2& GetMouseWorldCoords() const { return m_MouseWorldCoords; }

		inline void Activate() { m_bActivated = true; }
		inline void Deactivate() { m_bActivated = false; }
		inline const bool IsActivated() const { return m_bActivated; }
		inline const bool OutOfBounds() const { return m_bOutOfBounds; }
	};
}