#pragma once
#include <memory>
#include "../scene/Scene.h"

struct ImGuiContext;

namespace HybridPBR {

    class ImGuiComponentManager {
    public:
        ImGuiComponentManager();
        ~ImGuiComponentManager();

        // 组件渲染方法
        void ShowSceneHierarchy(std::unique_ptr<Scene>& scene);
        void ShowInspector(std::shared_ptr<SceneNode> selectedNode);
        void ShowSceneStats(std::unique_ptr<Scene>& scene);
        void ShowLightHierarchy(std::unique_ptr<Scene>& scene);
        
        // Gizmo渲染方法
        void RenderGizmo(const Camera& camera, const Scene& scene);
        void HandleGizmoInteraction(const Camera& camera, const Scene& scene, float deltaTime);

        // 设置当前选中的节点
        void SetSelectedNode(std::shared_ptr<SceneNode> node) { selectedNode = node; }
        std::shared_ptr<SceneNode> GetSelectedNode() const { return selectedNode; }
        
        // 设置当前选中的光源
        void SetSelectedLight(std::shared_ptr<Light> light) { selectedLight = light; }
        std::shared_ptr<Light> GetSelectedLight() const { return selectedLight; }

    private:
        std::shared_ptr<SceneNode> selectedNode;
        std::shared_ptr<Light> selectedLight;

        // Gizmo相关状态
        enum class GizmoAxis { NONE, X, Y, Z };
        GizmoAxis activeGizmoAxis = GizmoAxis::NONE;
        glm::vec3 gizmoInitialPosition;
        glm::vec2 mouseInitialPosition;

        bool IsPointInGizmoArea(const glm::vec3& worldPos, const glm::vec2& screenPos, float radius = 10.0f);

        // 内部辅助方法
        void DisplayNodeTree(std::shared_ptr<SceneNode> node, const Camera& camera);
        void DisplayLightTree(std::unique_ptr<Scene>& scene);
        void EditTransform(Transform& transform);
        void EditMaterial(std::shared_ptr<Material> material);
        void EditLight(std::shared_ptr<Light> light);
    };

} // namespace HybridPBR