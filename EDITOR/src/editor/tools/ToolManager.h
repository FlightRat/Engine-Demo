#pragma once
#include <entt.hpp>

namespace ENGINE_RENDERING { class Camera3D; }
namespace ENGINE_EDITOR {
	// class AbstractTool;
	// class TileTool;
	class Gizmo;
	// class SceneObject;
	// enum class EToolType;
	enum class EGizmoType;

	class ToolManager {
	private:
		// std::map<EToolType, std::unique_ptr<TileTool>> m_mapTools;
		std::map<EGizmoType, std::unique_ptr<Gizmo>> m_mapGizmos;

		// EToolType m_eActiveToolType;
		EGizmoType m_eActiveGizmoType;

	public:
		ToolManager();
		~ToolManager() = default;
		
		void Update();

		Gizmo* GetActiveGizmo();
		void SetGizmoActivate(EGizmoType eGizmoType);
		inline EGizmoType GetActiveGizmoType() const { return m_eActiveGizmoType; }

		void SetSelectedEntity(entt::entity entity);

	};
}

