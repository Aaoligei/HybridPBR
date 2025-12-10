#include "ImGuiComponentManager.h"
#include"resources/ResourceManager.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "../scene/Scene.h"
#include "../rendering/rasterization/Camera.h"
#include "../utils/MathUtils.h"
#include "../core/Input.h"

namespace HybridPBR {

    ImGuiComponentManager::ImGuiComponentManager() {
    }

    ImGuiComponentManager::~ImGuiComponentManager() {
    }

    void ImGuiComponentManager::ShowSceneHierarchy(std::unique_ptr<Scene>& scene) {
        ImGui::Begin("Scene Hierarchy");
        
        if (scene) {
            DisplayNodeTree(scene->GetRoot(),*scene->GetMainCamera());
        }
        
        ImGui::End();
    }

    void ImGuiComponentManager::ShowInspector(std::shared_ptr<SceneNode> node) {
        ImGui::Begin("Inspector");
        
        // 显示节点属性
        if (node) {
            ImGui::Text("Node: %s", node->GetName().c_str());
            ImGui::Separator();
            // 显示变换编辑器
            Transform& transform = node->GetTransform();
            EditTransform(transform);

            ImGui::Separator();
            //显示材质编辑器
            std::shared_ptr<Material> material = node->GetMaterial();
            EditMaterial(material);
        } 
        // 显示光源属性
        else if (selectedLight) {
            ImGui::Text("Light: %s", selectedLight->GetName().c_str());
            ImGui::Separator();
            EditLight(selectedLight);
        } 
        else {
            ImGui::Text("No object selected");
        }
        
        ImGui::End();
    }

    void ImGuiComponentManager::ShowSceneStats(std::unique_ptr<Scene>& scene) {
        ImGui::Begin("Scene Stats");
        
        if (scene) {
            ImGui::Separator();
            ImGui::Text("Nodes: %zu", scene->GetNodeCount());
            ImGui::Text("Lights: %zu", scene->GetAllLights().size());
            // ImGui::Text("Meshes: %zu", ResourceManager::GetInstance().GetMeshCount());
            // ImGui::Text("Materials: %zu", ResourceManager::GetInstance().GetMaterialCount());
        }
        
        ImGui::End();
    }

    void ImGuiComponentManager::ShowLightHierarchy(std::unique_ptr<Scene>& scene){
        ImGui::Begin("Light Hierarchy");
        
        if (scene) {
            DisplayLightTree(scene);
        }
        
        ImGui::End();
    }

    void ImGuiComponentManager::DisplayNodeTree(std::shared_ptr<SceneNode> node,const Camera& camera) {
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
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            selectedNode = node;
            // 重置gizmo状态
            activeGizmoAxis = GizmoAxis::NONE;
        }
        
        // 处理gizmo轴的选择
        Input& input = Input::GetInstance();
        if (selectedNode && input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            glm::vec3 nodePosition = selectedNode->GetTransform().GetPosition();
            
            // 转换到屏幕空间（简化版）
            glm::mat4 viewProj = camera.GetViewProjectionMatrix();
            glm::vec4 screenPosHomogeneous = viewProj * glm::vec4(nodePosition, 1.0f);
            if (screenPosHomogeneous.w > 0.0f) {
                glm::vec3 screenPos3D = screenPosHomogeneous / screenPosHomogeneous.w;
                glm::vec2 screenPos = glm::vec2(screenPos3D.x, -screenPos3D.y);
                screenPos = (screenPos + 1.0f) * 0.5f;
                screenPos *= glm::vec2(1280, 720);
                
                // 检查是否点击在某个轴上
                if (IsPointInGizmoArea(nodePosition, screenPos, 50.0f)) {
                    // 简化：根据鼠标位置判断哪个轴
                    Input& input = Input::GetInstance();
                    glm::vec2 mousePos(input.GetMouseX(), input.GetMouseY());
                    glm::vec2 delta = mousePos - screenPos;
                    
                    if (std::abs(delta.x) > std::abs(delta.y)) {
                        activeGizmoAxis = GizmoAxis::X; // X轴
                    } else {
                        activeGizmoAxis = GizmoAxis::Y; // Y轴
                    }
                    
                    gizmoInitialPosition = selectedNode->GetTransform().GetPosition();
                    mouseInitialPosition = mousePos;
                }
            }
        }
        
        if (hasChildren && opened && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
            for (const auto& child : node->GetChildren()) {
                DisplayNodeTree(child,camera);
            }
            ImGui::TreePop();
        }
    }

    void ImGuiComponentManager::DisplayLightTree(std::unique_ptr<Scene>& scene) {
        const auto& lights = scene->GetAllLights();
        
        ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        
        for (size_t i = 0; i < lights.size(); ++i) {
            const auto& light = lights[i];
            ImGuiTreeNodeFlags flags = baseFlags;
            
            bool isSelected = (selectedLight == light);
            if (isSelected) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            
            std::string label = light->GetName() + "##light" + std::to_string(i);
            ImGui::TreeNodeEx(label.c_str(), flags);
            
            // 处理光源选择
            if (ImGui::IsItemClicked()) {
                selectedLight = light;
                selectedNode.reset(); // 取消选择节点
            }
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

    void ImGuiComponentManager::EditMaterial(std::shared_ptr<Material> material) {
        if (!material) {
            ImGui::Text("No material assigned");
            return;
        }

        ImGui::Text("Material: %s", material->GetName().c_str());
        ImGui::Separator();

        auto& props = material->GetConstants();

        // Albedo color picker
        ImGui::Text("Albedo Color");
        ImGui::ColorEdit4("##Albedo", glm::value_ptr(props.albedoFactor));

        // Metallic slider
        ImGui::Text("Metallic");
        ImGui::SliderFloat("##Metallic", &props.metallicFactor, 0.0f, 1.0f);

        // Roughness slider
        ImGui::Text("Roughness");
        ImGui::SliderFloat("##Roughness", &props.roughnessFactor, 0.0f, 1.0f);

        // Ambient Occlusion slider
        ImGui::Text("Ambient Occlusion");
        ImGui::SliderFloat("##AO", &props.aoFactor, 0.0f, 1.0f);

        // Normal Scale slider
        ImGui::Text("Normal Scale");
        ImGui::SliderFloat("##NormalScale", &props.normalScale, 0.0f, 2.0f);

        // Emissive properties
        ImGui::Text("Emissive Intensity");
        ImGui::SliderFloat("##EmissiveIntensity", &props.emissiveIntensity, 0.0f, 5.0f);

        ImGui::Text("Emissive Color");
        ImGui::ColorEdit3("##EmissiveColor", glm::value_ptr(props.emissiveFactor));

        ImGui::Separator();
        ImGui::Text("Textures:");
        ImGui::Indent();

        ImGui::Unindent();
    }

    void ImGuiComponentManager::EditLight(std::shared_ptr<Light> light) {
        if (!light) return;

        // 显示光源类型
        const char* lightTypeStr = "Unknown";
        switch (light->GetType()) {
            case LightType::DIRECTIONAL:
                lightTypeStr = "Directional";
                break;
            case LightType::POINT:
                lightTypeStr = "Point";
                break;
            case LightType::SPOT:
                lightTypeStr = "Spot";
                break;
        }

        ImGui::Text("Type: %s", lightTypeStr);
        
        // 光源名称编辑
        static char nameBuffer[128];
        strcpy_s(nameBuffer, light->GetName().c_str());
        if (ImGui::InputText("Name", nameBuffer, IM_ARRAYSIZE(nameBuffer))) {
            light->SetName(std::string(nameBuffer));
        }

        // 启用状态
        bool enabled = light->IsEnabled();
        if (ImGui::Checkbox("Enabled", &enabled)) {
            light->SetEnabled(enabled);
        }

        LightProperties& props = light->GetProperties();

        // 颜色
        ImGui::Text("Color");
        ImGui::ColorEdit3("##LightColor", glm::value_ptr(props.color));

        // 强度
        ImGui::Text("Intensity");
        ImGui::DragFloat("##Intensity", &props.intensity, 0.1f, 0.0f, 100.0f);

        // 根据光源类型显示特定参数
        if (light->GetType() == LightType::POINT || light->GetType() == LightType::SPOT) {
            ImGui::Separator();
            ImGui::Text("Point/Spot Light Properties");

            // 范围
            ImGui::Text("Range");
            ImGui::DragFloat("##Range", &props.range, 0.1f, 0.1f, 1000.0f);

            // 衰减系数
            ImGui::Text("Constant");
            ImGui::DragFloat("##Constant", &props.constant, 0.01f, 0.0f, 10.0f);

            ImGui::Text("Linear");
            ImGui::DragFloat("##Linear", &props.linear, 0.01f, 0.0f, 10.0f);

            ImGui::Text("Quadratic");
            ImGui::DragFloat("##Quadratic", &props.quadratic, 0.01f, 0.0f, 10.0f);
        }

        if (light->GetType() == LightType::SPOT) {
            ImGui::Separator();
            ImGui::Text("Spot Light Properties");

            // 内外截角
            float innerCutoffDeg = glm::degrees(acos(props.innerCutoff));
            float outerCutoffDeg = glm::degrees(acos(props.outerCutoff));

            ImGui::Text("Inner Cutoff (degrees)");
            if (ImGui::DragFloat("##InnerCutoff", &innerCutoffDeg, 1.0f, 0.0f, 90.0f)) {
                props.innerCutoff = glm::cos(glm::radians(innerCutoffDeg));
            }

            ImGui::Text("Outer Cutoff (degrees)");
            if (ImGui::DragFloat("##OuterCutoff", &outerCutoffDeg, 1.0f, 0.0f, 90.0f)) {
                props.outerCutoff = glm::cos(glm::radians(outerCutoffDeg));
            }
        }

        // 位置和方向（这些通常由场景节点的变换控制，但对于光源我们也可以直接编辑）
        ImGui::Separator();
        ImGui::Text("Position/Directon");

        glm::vec3 position = light->GetPosition();
        if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f)) {
            light->SetPosition(position);
        }

        glm::vec3 direction = light->GetDirection();
        if (ImGui::DragFloat3("Direction", glm::value_ptr(direction), 0.1f)) {
            light->SetDirection(direction);
        }
    }

    void ImGuiComponentManager::RenderGizmo(const Camera& camera, const Scene& scene) {
        if (!selectedNode) return;

        // 获取选中物体的位置（世界空间）
        glm::vec3 nodePosition = selectedNode->GetTransform().GetPosition();
        
        // 转换到屏幕空间
        glm::mat4 viewProj = camera.GetViewProjectionMatrix();
        glm::vec4 screenPosHomogeneous = viewProj * glm::vec4(nodePosition, 1.0f);
        if (screenPosHomogeneous.w <= 0.0f) return; // 在相机后面
    
        glm::vec3 screenPos3D = screenPosHomogeneous / screenPosHomogeneous.w;
        glm::vec2 screenPos = glm::vec2(screenPos3D.x, -screenPos3D.y); // Y轴翻转
        screenPos = (screenPos + 1.0f) * 0.5f; // [-1,1] -> [0,1]
        screenPos *= glm::vec2(1280, 720); // 假设窗口大小

        // 绘制三个轴
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 center = ImVec2(screenPos.x, screenPos.y);
        float axisLength = 50.0f;
        
        // X轴（红色）
        ImVec2 xEnd = ImVec2(center.x + axisLength, center.y);
        drawList->AddLine(center, xEnd, IM_COL32(255, 0, 0, 255), 3.0f);
        
        // Y轴（绿色）
        ImVec2 yEnd = ImVec2(center.x, center.y - axisLength);
        drawList->AddLine(center, yEnd, IM_COL32(0, 255, 0, 255), 3.0f);
        
        // Z轴（蓝色）
        ImVec2 zEnd = ImVec2(center.x + axisLength * 0.707f, center.y - axisLength * 0.707f);
        drawList->AddLine(center, zEnd, IM_COL32(0, 0, 255, 255), 3.0f);
    }

    void ImGuiComponentManager::HandleGizmoInteraction(const Camera& camera, const Scene& scene, float deltaTime) {
        if (!selectedNode || activeGizmoAxis == GizmoAxis::NONE) return;

        Input& input = Input::GetInstance();
        glm::vec2 currentMouse = glm::vec2(input.GetMouseX(), input.GetMouseY());
        
        // 计算鼠标移动量
        glm::vec2 delta = currentMouse - mouseInitialPosition;
        
        // 根据激活的轴移动物体
        glm::vec3 moveDirection = glm::vec3(0.0f);
        switch (activeGizmoAxis) {
            case GizmoAxis::X:
                moveDirection = camera.GetRight();
                break;
            case GizmoAxis::Y:
                moveDirection = camera.GetUp();
                break;
            case GizmoAxis::Z:
                moveDirection = camera.GetFront();
                break;
            default:
                break;
        }
        
        // 将屏幕空间移动转换为世界空间移动
        float speed = 0.01f;
        glm::vec3 worldDelta = moveDirection * (delta.x * speed);
        
        // 更新物体位置
        Transform& transform = selectedNode->GetTransform();
        transform.SetPosition(gizmoInitialPosition + worldDelta);
    }

    bool ImGuiComponentManager::IsPointInGizmoArea(const glm::vec3& worldPos, const glm::vec2& screenPos, float radius) {
        Input& input = Input::GetInstance();
        glm::vec2 mousePos = glm::vec2(input.GetMouseX(), input.GetMouseY());
        return glm::length(mousePos - screenPos) < radius;
    }
} // namespace HybridPBR