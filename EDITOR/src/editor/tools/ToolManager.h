#pragma once
#include <entt.hpp>

namespace ENGINE_RENDERING { class Camera3D; }
namespace ENGINE_EDITOR {
	//class AbstractTool;
	//class Gizmo;
	//class SceneObject;
	//enum class EGizmoType;

	class ToolManager {
	private:
		//std::map<EGizmoType, std::unique_ptr<Gizmo>> m_mapGizom;
		//EGizmoType m_eActiveGizmoType;

	public:
		ToolManager();
		~ToolManager() = default;
		
		//void Update();

		//bool SetupTools(SceneObject* pSceneObject, ENGINE_RENDERING::Camera3D* pCamera);

		//void SetGizmoActive(EGizmoType eGizmoType);
		//Gizmo* GetActiveGizmo();
		//inline EGizmoType GetActiveGizmoType() const { return m_eActiveGizmoType; }

		//AbstractTool* GetActiveToolFromAbstract();

		//void SetSelectedEntity(entt::entity entity);
	};
}

