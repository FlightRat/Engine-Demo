#include "CoreEngineData.h"

namespace ENGINE_CORE {
	CoreEngineData::CoreEngineData(): 
		m_WindowWidth{ 600 },
		m_WindowHeight{ 600 },
		m_DeltaTime{ 0.f },
		m_Gravity{ 9.8f },
		m_PhysicsTimeStep{ 1.0f / 60.0f },
		//to be used
		m_bPhysicsEnabled{ true },
		m_bPhysicsPaused{ false },
		m_bRenderColliders{ false }
	{
	}

	CoreEngineData& CoreEngineData::GetInstance()
	{
		static CoreEngineData instance{};
		return instance;
	}

	void CoreEngineData::UpdateDeltaTime()
	{
		auto now = std::chrono::steady_clock::now();
		m_DeltaTime = std::chrono::duration<double>(now - m_LastUpdate).count();
		m_LastUpdate = now;
	}

	void CoreEngineData::SetWindowWidth(int windowWidth)
	{
		m_WindowWidth = windowWidth;
	}

	void CoreEngineData::SetWindowHeight(int windowHeight)
	{
		m_WindowHeight = windowHeight;
	}

}

