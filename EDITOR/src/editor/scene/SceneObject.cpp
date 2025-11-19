#include "SceneObject.h"

namespace ENGINE_EDITOR {

	SceneObject::SceneObject(const std::string& sceneName) :m_Registry{}, m_RuntimeRegistry{}, m_sSceneName{ sceneName }, m_bLoaded{ false }
	{
	}

	void SceneObject::CopySceneToRuntime()
	{
	}

	void SceneObject::ClearRuntimeScene()
	{
	}
}


