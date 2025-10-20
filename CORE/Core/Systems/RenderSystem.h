#pragma once
#include"../ECS/Registry.h"

namespace ENGINE_CORE::Systems {
	class RenderSystem
	{
	private:
		ENGINE_CORE::ECS::Registry& m_Registry;
	public:
		RenderSystem(ENGINE_CORE::ECS::Registry& registry);
		~RenderSystem() = default;
		void Render();
	};
}