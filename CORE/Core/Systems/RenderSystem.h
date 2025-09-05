#pragma once
#include"../ECS/Registry.h"

namespace CORE::Systems {
	class RenderSystem
	{
	private:
		CORE::ECS::Registry& m_Registry;
	public:
		RenderSystem(CORE::ECS::Registry& registry);
		~RenderSystem() = default;
		void Render();
	};
}