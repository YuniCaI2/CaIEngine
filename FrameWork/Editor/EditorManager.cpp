//
// Created by 51092 on 2025/11/1.
//

#include "EditorManager.h"
#include "imgui.h"
#include "imgui_internal.h"

void Editor::EditorManager::BuildDockLayout() {
    ImGuiID dockspace_id = ImGui::GetID(mainDockSpaceName.c_str());
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
    {
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
        ImGuiID dock_id_right = 0;
        ImGuiID dock_id_main = dockspace_id;
        ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Right, 0.40f,
            &dock_id_right, &dock_id_main);
        ImGuiID dock_id_left_top = 0;
        ImGuiID dock_id_left_bottom = 0;
        ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Down, 0.50f,
            &dock_id_left_top, &dock_id_left_bottom);
        ImGuiID dock_id_right_left = 0;
        ImGuiID dock_id_right_right = 0;
        ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Left, 0.50f,
            &dock_id_right_left, &dock_id_right_right);
        ImGui::DockBuilderDockWindow("Hierarchy", dock_id_right_left);
        ImGui::DockBuilderDockWindow("Inspector", dock_id_right_right);
        ImGui::DockBuilderDockWindow("Scene", dock_id_left_top);
        ImGui::DockBuilderDockWindow("File", dock_id_left_bottom);
        ImGui::DockBuilderFinish(dockspace_id);
    }
    // Submit dockspace
    ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);
}
