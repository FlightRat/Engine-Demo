#pragma once
#include <string>

namespace ENGINE_CORE::ECS{class Registry;}

namespace ENGINE_CORE::Loaders
{
	class SceneLoader
	{
	private:
		bool SaveSceneJSON(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile);
		bool LoadSceneJSON(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile);
		// TODO: save and load for lua serializer
	public:
		SceneLoader() = default;
		~SceneLoader() = default;

		bool SaveScene(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile, bool bUseJSON = false);
		bool LoadScene(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile, bool bUseJSON = false);
	};
}