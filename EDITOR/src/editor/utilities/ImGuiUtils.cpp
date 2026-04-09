#include "ImGuiUtils.h"
#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../tools/ToolContext.h"
#include "Core/ECS/Components/TransformComponent.h"
#include "Rendering/Core/Camera3D.h"

namespace ImGui {
    void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
    {
        ImGui::PushID(label.c_str());           // ID作用域
        ImGui::Columns(2);                      // 开始多列布局（这里2列）

        // 绘制第一列
        ImGui::SetColumnWidth(0, columnWidth);  // 设置左列宽度
        ImGui::Text(label.c_str());             // 在左列绘制文本

        ImGui::NextColumn();                    // 切换到下一列

        // 绘制第二列
        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());             // 压入三个被平均分配的元素宽度
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });     // 临时修改 ImGui 的样式变量，作用域到PopStyleVar()为止
        // 显示的大小定义
        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };
        // --- X Axis (Red) ---
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        if (ImGui::Button("X", buttonSize)) values.x = resetValue;
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        // --- Y Axis (Green) ---
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        if (ImGui::Button("Y", buttonSize)) values.y = resetValue;
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        // --- Z Axis (Blue) ---
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        if (ImGui::Button("Z", buttonSize)) values.z = resetValue;
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        // 结束样式修改
        ImGui::PopStyleVar();

        ImGui::Columns(1);      // 结束多列布局
        ImGui::PopID();
    }

    void ActiveButton(const char* label, ImVec2 size)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, BUTTON_HELD);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, BUTTON_HELD);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, BUTTON_HELD);
        ImGui::Button(label, size);
        ImGui::PopStyleColor(3);
    }

    void DisabledButton(const char* label, ImVec2 size, const std::string& disabledMsg)
    {
        ImGui::BeginDisabled();
        ImGui::Button(label, size);

        if (!disabledMsg.empty())
            ImGui::SetItemTooltip(disabledMsg.c_str());
        
        ImGui::EndDisabled();
    }

    void ActiveImageButton(const char* buttonId, ImTextureID textureID, ImVec2 size)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, BUTTON_HELD);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, BUTTON_HELD);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, BUTTON_HELD);
        ImGui::ImageButton(buttonId, textureID, size);
        ImGui::PopStyleColor(3);
    }

    void DisabledImageButton(const char* buttonId, ImTextureID textureID, ImVec2 size, const std::string& disabledMsg)
    {
        ImGui::BeginDisabled();
        ImGui::ImageButton(buttonId, textureID, size);

        if (!disabledMsg.empty())
            ImGui::SetItemTooltip(disabledMsg.c_str());

        ImGui::EndDisabled();
    }

    void DrawGizmo(std::shared_ptr<ENGINE_RENDERING::Camera3D> camera)
    {
        auto& toolCTX = TOOL_CTX();
        if (!toolCTX.selectedEntity || toolCTX.gizmoOp == ENGINE_EDITOR::EGizmoType::NO_GIZMO)
            return;

        auto& entity = *toolCTX.selectedEntity;
        if (!entity.GetRegistry().valid(entity.GetEntity()))
            return;

        if (!entity.HasComponent<ENGINE_CORE::ECS::TransformComponent>())
            return;

        auto& transform = entity.GetComponent<ENGINE_CORE::ECS::TransformComponent>();

        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();

        // 指定imguizmo绘制区域
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, transform.position);
        modelMatrix = modelMatrix * glm::toMat4(transform.rotation_quat); // 四元数→旋转矩阵
        modelMatrix = glm::scale(modelMatrix, transform.scale);

        float aspectRatio = windowSize.x / windowSize.y;
        glm::mat4 viewMatrix = camera->GetViewMatrix();
        glm::mat4 projMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

        ImGuizmo::OPERATION imguizmoOp;
        switch (toolCTX.gizmoOp)
        {
        case ENGINE_EDITOR::EGizmoType::TRANSLATE:
            imguizmoOp = ImGuizmo::TRANSLATE;
            break;
        case ENGINE_EDITOR::EGizmoType::ROTATE:
            imguizmoOp = ImGuizmo::ROTATE;
            break;
        case ENGINE_EDITOR::EGizmoType::SCALE:
            imguizmoOp = ImGuizmo::SCALE;
            break;
        case ENGINE_EDITOR::EGizmoType::NO_GIZMO:
            return;
        }

        ImGuizmo::MODE imguizmoMode = toolCTX.useWorldSpace ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

        bool manipulated = ImGuizmo::Manipulate(
            glm::value_ptr(viewMatrix),
            glm::value_ptr(projMatrix),
            imguizmoOp,
            imguizmoMode,
            glm::value_ptr(modelMatrix),
            nullptr,
            nullptr
        );

        if (manipulated)
        {
            glm::vec3 newPosition;
            glm::quat newRotationQuat;
            glm::vec3 newScale;
            glm::vec3 skew;
            glm::vec4 perspective;
            
            glm::decompose(modelMatrix, newScale, newRotationQuat, newPosition, skew, perspective);

            transform.position = newPosition;
            transform.rotation_quat = newRotationQuat;
            transform.rotation_eular = glm::degrees(glm::eulerAngles(transform.rotation_quat)); // 四元数→弧度→欧拉角
            transform.scale = newScale;

        }
    }

	void ColoredLabel(const std::string& label, const ImVec2& size, const ImVec4& color)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
		ImGui::Button(label.c_str(), size);
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
	}

	void OffsetTextX(const std::string& label, float position)
	{
		ImGui::SetCursorPosX(position);
		ImGui::Text(label.c_str());
	}

	void AddSpaces(int numSpaces)
	{
		assert(numSpaces > 0 && "Number of spaces must be a positive number!");
		for (int i = 0; i < numSpaces; ++i)
			ImGui::Spacing();
	}

	void InlineLabel(const std::string& label, float spaceSize)
	{
		ImGui::Text(label.c_str());
		ImGui::SameLine();
		ImGui::SetCursorPosX(spaceSize);
	}
}


