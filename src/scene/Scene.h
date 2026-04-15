#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "SceneNode.h"
#include "../rendering/common/Light.h"
#include "../rendering/rasterization/Camera.h"

namespace HybridPBR {

    class Scene {
    public:
        Scene();
        ~Scene();
        
        // 场景管理
        std::shared_ptr<SceneNode> CreateNode(const std::string& name = "Node");
        void AddNode(std::shared_ptr<SceneNode> node);
        void RemoveNode(SceneNode* node);
        
        std::shared_ptr<SceneNode> GetRoot() const { return root; }
        std::shared_ptr<SceneNode> FindNode(const std::string& name);
        
        // 获取节点总数
        size_t GetNodeCount() const { return nodeCount; }
        
        // 相机管理
        void SetMainCamera(std::shared_ptr<Camera> camera);
        std::shared_ptr<Camera> GetMainCamera() const { return mainCamera; }
        
        // 光源管理
        void AddLight(std::shared_ptr<Light> light);
        void RemoveLight(Light* light);
        const std::vector<std::shared_ptr<Light>>& GetLights() const { return lights; }
        
        // 更新
        void Update();
        
        // 脏标记管理
        void SetDirty();
        bool IsDirty() const { return sceneDirty; }
        void ClearDirtyFlag() { sceneDirty = false; }
        
        // 序列化
        bool SaveToFile(const std::string& filepath);
        bool LoadFromFile(const std::string& filepath);

    private:
        std::shared_ptr<SceneNode> root;
        std::shared_ptr<Camera> mainCamera;
        std::vector<std::shared_ptr<Light>> lights;
        
        std::unordered_map<std::string, std::shared_ptr<SceneNode>> nodeMap;
        size_t nodeCount = 0; // 节点计数器
        
        // 脏标记 - 用于通知渲染器场景是否发生变化
        bool sceneDirty = true;
        
        void BuildNodeMap(SceneNode* node);
    };

} // namespace HybridPBR