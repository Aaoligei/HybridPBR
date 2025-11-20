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
            ImGui::Text("Lights: %zu", scene->GetLights().size());
            ImGui::Text("Meshes: %zu", ResourceManager::GetInstance().GetMeshCount());
            ImGui::Text("Materials: %zu", ResourceManager::GetInstance().GetMaterialCount());
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
            selectedLight.reset(); // 取消选择光源
        }
        
        if (hasChildren && opened && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
            for (const auto& child : node->GetChildren()) {
                DisplayNodeTree(child);
            }
            ImGui::TreePop();
        }
    }

    void ImGuiComponentManager::DisplayLightTree(std::unique_ptr<Scene>& scene) {
        const auto& lights = scene->GetLights();
        
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

        MaterialProperties& props = material->GetProperties();
        ImGui::Text("Shader Name: %s", material->GetCustomShaderName().c_str());
        ImGui::Text("Shader Type: %s", material->GetShaderTypeString().c_str());


        // Albedo color picker
        ImGui::Text("Albedo Color");
        ImGui::ColorEdit4("##Albedo", glm::value_ptr(props.albedo));

        // Metallic slider
        ImGui::Text("Metallic");
        ImGui::SliderFloat("##Metallic", &props.metallic, 0.0f, 1.0f);

        // Roughness slider
        ImGui::Text("Roughness");
        ImGui::SliderFloat("##Roughness", &props.roughness, 0.0f, 1.0f);

        // Ambient Occlusion slider
        ImGui::Text("Ambient Occlusion");
        ImGui::SliderFloat("##AO", &props.ambientOcclusion, 0.0f, 1.0f);

        // Normal Scale slider
        ImGui::Text("Normal Scale");
        ImGui::SliderFloat("##NormalScale", &props.normalScale, 0.0f, 2.0f);

        // Emissive properties
        ImGui::Text("Emissive Intensity");
        ImGui::SliderFloat("##EmissiveIntensity", &props.emissiveIntensity, 0.0f, 5.0f);

        ImGui::Text("Emissive Color");
        ImGui::ColorEdit3("##EmissiveColor", glm::value_ptr(props.emissiveColor));

        // Texture scale and offset
        ImGui::Text("Texture Scale");
        ImGui::DragFloat2("##TextureScale", glm::value_ptr(props.textureScale), 0.1f);

        ImGui::Text("Texture Offset");
        ImGui::DragFloat2("##TextureOffset", glm::value_ptr(props.textureOffset), 0.1f);

        ImGui::Separator();
        ImGui::Text("Textures:");
        ImGui::Indent();

        // 显示各种类型的纹理
        const char* textureTypes[] = { 
            "Diffuse", "Specular", "Normal", "Height", "Roughness", 
            "Metallic", "Ambient Occlusion", "Emissive", "HDR", "Cubemap" 
        };

        for (int i = 0; i < static_cast<int>(TextureType::CUBEMAP) + 1; ++i) {
            TextureType type = static_cast<TextureType>(i);
            std::shared_ptr<Texture> texture = material->GetTexture(type);
            
            ImGui::Text("%s: ", textureTypes[i]);
            ImGui::SameLine();
            
            if (texture) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Assigned (%dx%d)", 
                                 texture->GetWidth(), texture->GetHeight());
                
                // 添加纹理预览按钮
                ImGui::SameLine();
                std::string buttonLabel = std::string("Preview##") + textureTypes[i] + std::to_string(i);
                if (ImGui::SmallButton(buttonLabel.c_str())) {
                    // 纹理预览功能可以在这里实现
                    // 当前只是占位符，后续可以添加实际的纹理查看器
                }
                
                // 显示纹理文件路径
                if (!texture->GetFilePath().empty()) {
                    ImGui::Text("    Path: %s", texture->GetFilePath().c_str());
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "None");
            }
        }

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

} // namespace HybridPBR