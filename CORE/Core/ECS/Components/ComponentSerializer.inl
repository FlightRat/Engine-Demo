#pragma once
#include "ComponentSerializer.h"

namespace ENGINE_CORE::ECS {

	template<typename TComponent, typename TSerializer>
	inline void ComponentSerializer::Serialize(TSerializer& serializer, const TComponent& component)
	{
		SerializeComponent(serializer, component);
	}

	template<typename TComponent, typename TTable>
	inline auto ComponentSerializer::Deserialize(const TTable& table)
	{
		if constexpr (std::is_same_v <TComponent, TransformComponent>)
		{
			return DeserializeTransform(table);
		}
		else if constexpr (std::is_same_v<TComponent, PhysicsComponent>)
		{
			return DeserializePhysics(table);
		}
		else if constexpr (std::is_same_v<TComponent, MeshFilter>)
		{
			return DeserializeMeshFilter(table);
		}
		else if constexpr (std::is_same_v<TComponent, MeshRender>)
		{
			return DeserializeMeshRender(table);
		}
		else
		{
			static_assert(false, "Cant't deserialize invalid component!");
		}
	}

}
