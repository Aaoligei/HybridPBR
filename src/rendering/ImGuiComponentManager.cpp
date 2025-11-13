#include "ImGuiComponentManager.h"
#include"resources/ResourceManager.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace HybridPBR {

    ImGuiComponentManager::ImGuiComponentManager() {
    }

    ImGuiComponentManager::~ImGuiComponentManager() {
    }

    void ImGuiComponentManager::ShowSceneHierarchy(std::unique_ptr<Scene>& scene) {
        ImGui::Begin("Scene Hierarchy");
        
        if (scene) {
            DisplayNodeTree(scene->GetRoot());
        }
        
        ImGui::End();
    }

    void ImGuiComponentManager::ShowTransformEditor(std::shared_ptr<SceneNode> node) {
        ImGui::Begin("Transform Editor");
        
        if (node) {
            ImGui::Text("Node: %s", node->GetName().c_str());
            ImGui::Separator();
            
            Transform& transform = node->GetTransform();
            EditTransform(transform);
        } else {
            ImGui::Text("No node selected");
        }
        
        ImGui::End();
    }

    void ImGuiComponentManager::ShowSceneStats(std::unique_ptr<Scene>& scene) {
        ImGui::Begin("Scene Stats");
        
        if (scene) {
            ImGui::Text("Nodes: %zu", scene->GetNodeCount());
            ImGui::Text("Lights: %zu", scene->GetLights().size());
            ImGui::Text("Meshes: %zu", ResourceManager::GetInstance().GetMeshCount());
            ImGui::Text("Materials: %zu", ResourceManager::GetInstance().GetMaterialCount());
        }
        
        ImGui::End();
    }

    void ImGuiComponentManager::DisplayNodeTree(std::shared_ptr<SceneNode> node) {
        if (!node) return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        bool hasChildren = !node->GetChildren().empty();
        
        if (!hasChildren) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        
        bool isSelected = (selectedNode == node);
        if (isSelected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        
        bool opened = ImGui::TreeNodeEx(node->GetName().c_str(), flags);
        
        // 处理节点选择
        if (ImGui::IsItemClicked()) {
            selectedNode = node;
        }
        
        if (hasChildren && opened && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
            for (const auto& child : node->GetChildren()) {
                DisplayNodeTree(child);
            }
            ImGui::TreePop();
        }
    }

    void ImGuiComponentManager::EditTransform(Transform& transform) {
        glm::vec3 position = transform.GetPosition();
        glm::vec3 rotation = transform.GetRotation();
        glm::vec3 scale = transform.GetScale();
        
        bool changed = false;
        
        // 位置编辑
        if (ImGui::TreeNode("Position")) {
            changed |= ImGui::DragFloat3("##Position", glm::value_ptr(position), 0.1f);
            ImGui::TreePop();
        }
        
        // 旋转编辑
        if (ImGui::TreeNode("Rotation")) {
            changed |= ImGui::DragFloat3("##Rotation", glm::value_ptr(rotation), 1.0f);
            ImGui::TreePop();
        }
        
        // 缩放编辑
        if (ImGui::TreeNode("Scale")) {
            changed |= ImGui::DragFloat3("##Scale", glm::value_ptr(scale), 0.1f);
            ImGui::TreePop();
        }
        
        // 如果有更改，更新变换
        if (changed) {
            transform.SetPosition(position);
            transform.SetRotation(rotation);
            transform.SetScale(scale);
        }
    }

} // namespace HybridPBR