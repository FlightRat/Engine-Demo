#include "SceneLoader.h"

namespace ENGINE_CORE::Loaders {
	bool SceneLoader::SaveSceneJSON(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile)
	{
		return false;
	}

	bool SceneLoader::LoadSceneJSON(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile)
	{
		return false;
	}

	bool SceneLoader::SaveScene(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile, bool bUseJSON)
	{
		if (bUseJSON)
			return SaveSceneJSON(registry, sSceneFile);
		return false;
	}

	bool SceneLoader::LoadScene(ENGINE_CORE::ECS::Registry& registry, const std::string& sSceneFile, bool bUseJSON)
	{
		if (bUseJSON)
			return LoadSceneJSON(registry, sSceneFile);
		return false;
	}
}


