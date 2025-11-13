#include "SceneNode.h"

namespace HybridPBR {

    SceneNode::SceneNode(const std::string& nodeName) 
        : name(nodeName) {
    }

    SceneNode::~SceneNode() {
        // 自动清理子节点
        children.clear();
    }

    void SceneNode::AddChild(std::shared_ptr<SceneNode> child) {
        if (!child) return;
        
        child->parent = this;
        children.push_back(child);
    }

    void SceneNode::RemoveChild(SceneNode* childToRemove) {
        for (auto it = children.begin(); it != children.end(); ++it) {
            if (it->get() == childToRemove) {
                (*it)->parent = nullptr;
                children.erase(it);
                break;
            }
        }
    }

    std::shared_ptr<SceneNode> SceneNode::GetChild(const std::string& childName) {
        for (auto& child : children) {
            if (child->GetName() == childName) {
                return child;
            }
            
            // 递归搜索
            auto found = child->GetChild(childName);
            if (found) {
                return found;
            }
        }
        
        return nullptr;
    }

    void SceneNode::SetMesh(std::shared_ptr<Mesh> newMesh) {
        mesh = newMesh;
    }

    void SceneNode::SetMaterial(std::shared_ptr<Material> newMaterial) {
        material = newMaterial;
    }

    void SceneNode::Update() {
        // 更新变换
        transform.GetWorldMatrix(); // 确保世界矩阵是最新的
        
        // 更新所有子节点
        for (auto& child : children) {
            child->Update();
        }
    }

} // namespace HybridPBR