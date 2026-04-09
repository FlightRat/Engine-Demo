#pragma once
#include <memory>
#include "Core/ECS/Entity.h"

namespace ENGINE_EDITOR {
	enum class EGizmoType
	{
		TRANSLATE = 0,
		ROTATE,
		SCALE,

		NO_GIZMO
	};

	struct ToolContext
	{
		static ToolContext& Get() {
			static ToolContext instance;
			return instance;
		}

		// 当前选中的实体（弱引用，避免循环持有）
		std::shared_ptr<ENGINE_CORE::ECS::Entity> selectedEntity{ nullptr };

		// 当前 Gizmo 操作模式
		EGizmoType gizmoOp{ EGizmoType::NO_GIZMO };

		// 是否使用世界坐标系（true=世界，false=本地）
		bool useWorldSpace{ true };

	private:
		ToolContext() = default;
	};
}

#define TOOL_CTX() ENGINE_EDITOR::ToolContext::Get()
