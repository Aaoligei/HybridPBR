#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Transform.h"
#include "../rendering/rasterization/Mesh.h"
#include "../rendering/common/Material.h"

namespace HybridPBR {

    class SceneNode {
    public:
        SceneNode(const std::string& name = "Node");
        ~SceneNode();
        
        // 节点操作
        void AddChild(std::shared_ptr<SceneNode> child);
        void RemoveChild(SceneNode* child);
        std::shared_ptr<SceneNode> GetChild(const std::string& name);
        
        // 渲染组件
        void SetMesh(std::shared_ptr<Mesh> mesh);
        void SetMaterial(std::shared_ptr<Material> material);
        
        // 获取组件
        const std::string& GetName() const { return name; }
        Transform& GetTransform() { return transform; }
        const Transform& GetTransform() const { return transform; }
        std::shared_ptr<Mesh> GetMesh() const { return mesh; }
        std::shared_ptr<Material> GetMaterial() const { return material; }
        
        // 层次遍历
        const std::vector<std::shared_ptr<SceneNode>>& GetChildren() const { return children; }
        SceneNode* GetParent() const { return parent; }
        
        // 更新
        void Update();

    private:
        std::string name;
        Transform transform;
        
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
        
        SceneNode* parent = nullptr;
        std::vector<std::shared_ptr<SceneNode>> children;
    };

} // namespace HybridPBR