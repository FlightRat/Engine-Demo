#include "../scene/PlaybackController.h"
#include <imgui.h>
#include "ComponentDrawer.h"
#include "Core/ECS/Entity.h"

namespace ENGINE_EDITOR {

	template<typename Tcomponent>
	inline void ComponentDrawer::DrawEntityComponentInfo(ENGINE_CORE::ECS::Entity& entity)
	{
		auto& component = entity.GetComponent<Tcomponent>();
		if (!PlaybackController::Get().CanEdit()) {
            auto snapshot = component;
            ImGui::BeginDisabled(); DrawImGuiComponent(snapshot); ImGui::EndDisabled();
        } else DrawImGuiComponent(component);
	}

	template<typename TComponent>
	inline void ComponentDrawer::DrawComponentInfo(TComponent& component)
	{
		if (!PlaybackController::Get().CanEdit()) {
            auto snapshot = component;
            ImGui::BeginDisabled(); DrawImGuiComponent(snapshot); ImGui::EndDisabled();
        } else DrawImGuiComponent(component);
	}

	template<typename TComponent>
	inline void ComponentDrawer::RegisterUIComponent()
	{
		using namespace entt::literals;

		entt::meta_factory<TComponent>()
			.type(entt::type_hash<TComponent>::value())
			.template func<&DrawEntityComponentInfo<TComponent>>("DrawEntityComponentInfo"_hs);
	}

}
