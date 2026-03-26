#include "AbstractTool.h"
#include "Logger/Logger.h"
#include "Core/Inputs/InputManager.h"
#include "Core/ECS/Registry.h"
#include "Rendering/Core/Camera3D.h"
#include "../utilities/editor_utilities.h"

namespace ENGINE_EDITOR {

	AbstractTool::AbstractTool() :
		m_MouseScreenCoords{ 0.0f },
		m_MouseWorldCoords{ 0.0f },
		m_GUICursorCoords{ 0.0f },
		m_GUIRelativeCoords{ 0.0f },
		m_WindowPos{ 0.0f },
		m_WindowSize{ 0.0f },
		m_bActivated{ false },
		m_bOutOfBounds{ false }
	{
	}

	bool AbstractTool::MouseBtnJustPressed(EMouseButton eButton)
	{
		return INPUT_MANAGER().GetMouse().IsBtnJustPressed(static_cast<int>(eButton));
	}
	bool AbstractTool::MouseBtnJustReleased(EMouseButton eButton)
	{
		return INPUT_MANAGER().GetMouse().IsBtnJustReleased(static_cast<int>(eButton));
	}
	bool AbstractTool::MouseBtnPressed(EMouseButton eButton)
	{
		return INPUT_MANAGER().GetMouse().IsBtnPressed(static_cast<int>(eButton));
	}
	bool AbstractTool::MouseMoving()
	{
		return INPUT_MANAGER().GetMouse().IsMouseMoving();
	}

	void AbstractTool::Update()
	{
		CheckOutOfBounds();
		UpdateMouseWorldCoords();
	}
	void AbstractTool::UpdateMouseWorldCoords()
	{
		m_MouseScreenCoords = m_GUICursorCoords - m_GUIRelativeCoords;
		if (!m_pCamera)
			return;
		//TODO -- m_MouseWorldCoords = m_pCamera->ScreenCoordsToWorld(m_MouseScreenCoords);
	}
	void AbstractTool::CheckOutOfBounds()
	{
		//TODO
	}

	bool AbstractTool::SetupTool(ENGINE_CORE::ECS::Registry* pRegistry, ENGINE_RENDERING::Camera3D* pCamera)
	{
		if (!pRegistry)
		{
			ENGINE_ERROR("Failed to setup tool -- Registry is nullptr!");
		}
		if (!pCamera)
		{
			ENGINE_ERROR("Failed to setup tool -- Camera is nullptr!");
		}

		m_pRegistry = pRegistry;
		m_pCamera = pCamera;

		return true;
	}
}


