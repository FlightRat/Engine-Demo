#pragma once
#include "Core/ECS/Entity.h"

namespace ENGINE_EDITOR {
	class SceneObject
	{
	private:
		ENGINE_CORE::ECS::Registry m_Registry, m_RuntimeRegistry;
		std::string m_sSceneName;
		bool m_bLoaded;
	public:
		SceneObject(const std::string& sceneName);
		~SceneObject() = default;

		void CopySceneToRuntime();
		void ClearRuntimeScene();
		
		inline const bool CheckLoad() { return m_bLoaded; }
		inline void SetLoad(const bool load) { m_bLoaded = load; }

		inline const std::string& GetName() { return m_sSceneName; }
		inline ENGINE_CORE::ECS::Registry& GetRegistry() { return m_Registry; }
		inline ENGINE_CORE::ECS::Registry* GetRegistryPtr() { return &m_Registry; }
		inline ENGINE_CORE::ECS::Registry& GetRuntimeRegistry() { return m_RuntimeRegistry; }
	};
}