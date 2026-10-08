#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#define SCENE_MANAGER() ENGINE_EDITOR::SceneManager::GetInstance()

namespace ENGINE_EDITOR {
	class SceneObject;

	class SceneManager
	{
	private:
		std::map<std::string, std::shared_ptr<ENGINE_EDITOR::SceneObject>> m_mapScene;
		std::string m_sCurrentScene{ "" };
	private:
		SceneManager() = default;
		~SceneManager() = default;
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;

	public:
		static SceneManager& GetInstance();

		bool AddScene(const std::string& sSceneName);
		bool DeleteScene(const std::string& sceneName);
		void CleanUp();
		bool ChangeSceneName(const std::string& sOldName, const std::string& sNewName);
		bool CheckHasScene(const std::string& sceneName);
		std::shared_ptr<ENGINE_EDITOR::SceneObject> GetScene(const std::string& sSceneName);
		std::shared_ptr<ENGINE_EDITOR::SceneObject>	GetCurrentScene();
		std::vector<std::string> GetAllSceneNames() const;
		inline void SetCurrentScene(const std::string& sSceneName) { m_sCurrentScene = sSceneName; }
		inline const std::string& GetCurrentSceneName() const { return m_sCurrentScene; }
	};
}