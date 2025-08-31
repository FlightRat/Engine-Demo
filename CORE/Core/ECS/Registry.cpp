#include "Registry.h"

CORE::ECS::Registry::Registry():m_pRegistry{ nullptr }
{
	m_pRegistry = std::make_unique<entt::registry>();
}
