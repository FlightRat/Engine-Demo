#include "SceneManager.h"
#include "Logger/Logger.h"
#include "SceneObject.h"
#include "UTILITIES/EngineUtilities.h"

namespace ENGINE_EDITOR {
	SceneManager& SceneManager::GetInstance()
	{
		static SceneManager instance{};
		return instance;
	}

	bool SceneManager::AddScene(const std::string& sSceneName)
	{
		if (m_mapScene.contains(sSceneName))
		{
			ENGINE_ERROR("Failed to add scene [{}] -- already exists!", sSceneName);
			return false;
		}
		auto [itr, bSuccess] = m_mapScene.emplace(sSceneName, std::move(std::make_shared<SceneObject>(sSceneName)));
		return bSuccess;
	}

	bool SceneManager::DeleteScene(const std::string& sceneName)
	{
		if (sceneName == SCENE_MANAGER().GetCurrentSceneName())
			return false;
		else
			return std::erase_if(m_mapScene, [&](const auto& pair) {return pair.first == sceneName; }) > 0;
	}

	bool SceneManager::ChangeSceneName(const std::string& sOldName, const std::string& sNewName)
	{
		return ENGINE_UTIL::ChangeKey(m_mapScene, sOldName, sNewName);
	}

	bool SceneManager::CheckHasScene(const std::string& sceneName)
	{
		return m_mapScene.contains(sceneName);
	}

	std::shared_ptr<ENGINE_EDITOR::SceneObject> SceneManager::GetScene(const std::string& sSceneName)
	{
		auto sceneItr = m_mapScene.find(sSceneName);
		if (sceneItr == m_mapScene.end())
		{
			ENGINE_ERROR("Failed to get scene [{}] -- does not exists!", sSceneName);
			return nullptr;
		}
		return sceneItr->second;
	}

	std::shared_ptr<ENGINE_EDITOR::SceneObject> SceneManager::GetCurrentScene()
	{
		if (m_sCurrentScene.empty())
			return nullptr;

		auto sceneItr = m_mapScene.find(m_sCurrentScene);
		if (sceneItr == m_mapScene.end())
		{
			ENGINE_ERROR("Failed to get scene [{}] -- does not exists!", m_sCurrentScene);
			return nullptr;
		}
		return sceneItr->second;
	}

	std::vector<std::string> SceneManager::GetAllSceneNames() const
	{
		return ENGINE_UTIL::GetKeys(m_mapScene);
	}

	void SceneManager::CleanUp()
	{
		m_mapScene.clear();
		m_sCurrentScene.clear();
	}
}

