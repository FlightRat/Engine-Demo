#include "SceneObject.h"
#include "Core/ECS/MetaUtilities.h"
#include "Core/ECS/Components/Identification.h"
#include "Core/ECS/Components/ScriptComponent.h"
#include "Core/ECS/Components/TransformComponent.h"
#include "Core/ECS/Components/PhysicsComponent.h"
#include "Core/ECS/Components/MeshFilter.h"
#include "Core/ECS/Components/MeshRender.h"

using namespace entt::literals;
using namespace ENGINE_CORE::ECS;

namespace ENGINE_EDITOR {

	SceneObject::SceneObject(const std::string& sceneName) :m_Registry{}, m_RuntimeRegistry{}, m_sSceneName{ sceneName }, m_bPlay{ false }
	{
	}

	void SceneObject::CopySceneToRuntime()
	{
		auto& sourceRegistry = m_Registry.GetRegistry();
		for (auto sourceEntity : sourceRegistry.view<entt::entity>(entt::exclude< ScriptComponent>))
		{
			entt::entity targetEntity = m_RuntimeRegistry.CreateEntity();
			for (auto&& [id, storage] : sourceRegistry.storage())
			{
				if (!storage.contains(sourceEntity))
					continue;
				ENGINE_CORE::Utils::InvokeMetaFunction(
					id, "copy_component"_hs,
					ENGINE_CORE::ECS::Entity{ m_Registry,sourceEntity }, 
					ENGINE_CORE::ECS::Entity{m_RuntimeRegistry,targetEntity}
				);
			}
		}
	}

	void SceneObject::ClearRuntimeScene()
	{
		m_RuntimeRegistry.ClearRegistry();
	}
}


