#pragma once
#include<string>
#include<chrono>

#define CORE_GLOBALS() ENGINE_CORE::CoreEngineData::GetInstance()

namespace ENGINE_CORE {
	class CoreEngineData
	{
	private:
		int m_WindowWidth;
		int m_WindowHeight;

		double m_DeltaTime;
		double m_Accumulator = { 0 };
		std::chrono::steady_clock::time_point m_LastUpdate;

		float m_Gravity;
		float m_PhysicsTimeStep;
		bool m_bPhysicsEnabled;
		bool m_bPhysicsPaused;
		bool m_bRenderColliders;

		std::string m_sProjectPath;

	private:
		CoreEngineData();
		~CoreEngineData() = default;
		CoreEngineData(const CoreEngineData&) = delete;
		CoreEngineData& operator=(const CoreEngineData&) = delete;

	public:
		static CoreEngineData& GetInstance();
		
		void UpdateDeltaTime();
		inline double GetDeltaTime() const { return m_DeltaTime; }
		inline double& GetAccumulator() { return m_Accumulator; }

		void SetWindowWidth(int windowWidth);
		void SetWindowHeight(int windowHeight);
		inline int WindowWidth() const { return m_WindowWidth; }
		inline int WindowHeight() const { return m_WindowHeight; }

		inline void EnableColliderRender() { m_bRenderColliders = true; }
		inline void DisableColliderRender() { m_bRenderColliders = false; }
		inline bool RenderCollidersEnabled() const { return m_bRenderColliders; }
		inline void ToggleRenderCollisions() { m_bRenderColliders = !m_bRenderColliders; }

		// Physics 
		inline void SetGravity(float gravity) { m_Gravity = gravity; }
		inline void SetPhysicsTimeStep(float timestep) { m_PhysicsTimeStep = timestep; }
		inline float GetGravity() const { return m_Gravity; }
		inline float GetPhysicsTimeStep() const { return m_PhysicsTimeStep; }
		inline void EnablePhysics() { m_bPhysicsEnabled = true; }
		inline void DisablePhysics() { m_bPhysicsEnabled = false; }
		inline void PausePhysics() { m_bPhysicsPaused = true; }
		inline void UnPausePhysics() { m_bPhysicsPaused = false; }
		inline const bool IsPhysicsEnabled() const { return m_bPhysicsEnabled; }
		inline const bool IsPhysicsPaused() const { return m_bPhysicsPaused; }

		inline const std::string& GetProjectPath() const { return m_sProjectPath; }
		inline void SetProjectPath(const std::string& sPath) { m_sProjectPath = sPath; }

	};
}