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
        void ShowTransformEditor(std::shared_ptr<SceneNode> selectedNode);
        void ShowSceneStats(std::unique_ptr<Scene>& scene);

        // 设置当前选中的节点
        void SetSelectedNode(std::shared_ptr<SceneNode> node) { selectedNode = node; }
        std::shared_ptr<SceneNode> GetSelectedNode() const { return selectedNode; }

    private:
        std::shared_ptr<SceneNode> selectedNode;

        // 内部辅助方法
        void DisplayNodeTree(std::shared_ptr<SceneNode> node);
        void EditTransform(Transform& transform);
    };

} // namespace HybridPBR