#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <queue>
#include "SceneNode.h"
#include "../rendering/common/Light.h"
#include "../rendering/rasterization/Camera.h"
#include "../core/Result.h"

namespace HybridPBR {

    /**
     * @brief 场景系统接口
     * 定义场景管理的高级接口
     * 
     * 修改理由：
     * 1. 分离场景逻辑和渲染逻辑，提高可测试性
     * 2. 提供事件系统，支持组件间解耦通信
     * 3. 实现场景分页和LOD系统，支持大型场景
     * 4. 添加查询系统，优化场景遍历性能
     */
    class ISceneSystem {
    public:
        virtual ~ISceneSystem() = default;
        
        // 场景生命周期
        virtual Result<void> Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual void Update(float deltaTime) = 0;
        
        // 节点管理
        virtual Result<std::shared_ptr<SceneNode>> CreateNode(const std::string& name = "Node") = 0;
        virtual Result<void> AddNode(std::shared_ptr<SceneNode> node) = 0;
        virtual Result<void> RemoveNode(const std::string& name) = 0;
        virtual std::shared_ptr<SceneNode> GetRoot() const = 0;
        virtual std::shared_ptr<SceneNode> FindNode(const std::string& name) = 0;
        
        // 相机管理
        virtual void SetMainCamera(std::shared_ptr<Camera> camera) = 0;
        virtual std::shared_ptr<Camera> GetMainCamera() const = 0;
        virtual std::vector<std::shared_ptr<Camera>> GetAllCameras() const = 0;
        
        // 光源管理
        virtual Result<void> AddLight(std::shared_ptr<Light> light) = 0;
        virtual Result<void> RemoveLight(const std::string& name) = 0;
        virtual std::shared_ptr<Light> GetLight(const std::string& name) const = 0;
        virtual std::vector<std::shared_ptr<Light>> GetAllLights() const = 0;
        virtual std::vector<std::shared_ptr<Light>> GetVisibleLights(const Camera& camera) const = 0;
        
        // 场景查询系统
        virtual std::vector<std::shared_ptr<SceneNode>> QueryNodes(const std::function<bool(const SceneNode&)>& predicate) const = 0;
        virtual std::vector<std::shared_ptr<SceneNode>> GetNodesInFrustum(const Camera& camera) const = 0;
        virtual std::vector<std::shared_ptr<SceneNode>> GetNodesInSphere(const glm::vec3& center, float radius) const = 0;
        virtual std::vector<std::shared_ptr<SceneNode>> GetNodesInBox(const glm::vec3& min, const glm::vec3& max) const = 0;
        
        // 场景状态
        virtual void SetDirty() = 0;
        virtual bool IsDirty() const = 0;
        virtual void ClearDirtyFlag() = 0;
        virtual size_t GetNodeCount() const = 0;
        
        // 序列化
        virtual Result<void> SaveToFile(const std::string& filepath) = 0;
        virtual Result<void> LoadFromFile(const std::string& filepath) = 0;
        
        // 事件系统
        using NodeEventCallback = std::function<void(std::shared_ptr<SceneNode>)>;
        virtual void RegisterNodeAddedCallback(NodeEventCallback callback) = 0;
        virtual void RegisterNodeRemovedCallback(NodeEventCallback callback) = 0;
        virtual void UnregisterNodeCallbacks() = 0;
    };

    /**
     * @brief 场景系统实现
     * 提供完整的场景管理功能
     */
    class SceneSystem : public ISceneSystem {
    public:
        SceneSystem();
        ~SceneSystem() override;
        
        // ISceneSystem接口实现
        Result<void> Initialize() override;
        void Shutdown() override;
        void Update(float deltaTime) override;
        
        Result<std::shared_ptr<SceneNode>> CreateNode(const std::string& name = "Node") override;
        Result<void> AddNode(std::shared_ptr<SceneNode> node) override;
        Result<void> RemoveNode(const std::string& name) override;
        std::shared_ptr<SceneNode> GetRoot() const override { return root_; }
        std::shared_ptr<SceneNode> FindNode(const std::string& name) override;
        
        void SetMainCamera(std::shared_ptr<Camera> camera) override;
        std::shared_ptr<Camera> GetMainCamera() const override { return mainCamera_; }
        std::vector<std::shared_ptr<Camera>> GetAllCameras() const override;
        
        Result<void> AddLight(std::shared_ptr<Light> light) override;
        Result<void> RemoveLight(const std::string& name) override;
        std::shared_ptr<Light> GetLight(const std::string& name) const override;
        std::vector<std::shared_ptr<Light>> GetAllLights() const override { return lights_; }
        std::vector<std::shared_ptr<Light>> GetVisibleLights(const Camera& camera) const override;
        
        std::vector<std::shared_ptr<SceneNode>> QueryNodes(const std::function<bool(const SceneNode&)>& predicate) const override;
        std::vector<std::shared_ptr<SceneNode>> GetNodesInFrustum(const Camera& camera) const override;
        std::vector<std::shared_ptr<SceneNode>> GetNodesInSphere(const glm::vec3& center, float radius) const override;
        std::vector<std::shared_ptr<SceneNode>> GetNodesInBox(const glm::vec3& min, const glm::vec3& max) const override;
        
        void SetDirty() override { sceneDirty_ = true; }
        bool IsDirty() const override { return sceneDirty_; }
        void ClearDirtyFlag() override { sceneDirty_ = false; }
        size_t GetNodeCount() const override { return nodeCount_; }
        
        Result<void> SaveToFile(const std::string& filepath) override;
        Result<void> LoadFromFile(const std::string& filepath) override;
        
        void RegisterNodeAddedCallback(NodeEventCallback callback) override { nodeAddedCallbacks_.push_back(callback); }
        void RegisterNodeRemovedCallback(NodeEventCallback callback) override { nodeRemovedCallbacks_.push_back(callback); }
        void UnregisterNodeCallbacks() override { nodeAddedCallbacks_.clear(); nodeRemovedCallbacks_.clear(); }

    private:
        std::shared_ptr<SceneNode> root_;
        std::shared_ptr<Camera> mainCamera_;
        std::vector<std::shared_ptr<Light>> lights_;
        std::unordered_map<std::string, std::shared_ptr<SceneNode>> nodeMap_;
        std::unordered_map<std::string, std::shared_ptr<Light>> lightMap_;
        size_t nodeCount_ = 0;
        bool sceneDirty_ = true;
        bool initialized_ = false;
        
        // 事件回调
        std::vector<NodeEventCallback> nodeAddedCallbacks_;
        std::vector<NodeEventCallback> nodeRemovedCallbacks_;
        
        // 内部方法
        void BuildNodeMap(SceneNode* node);
        void CountNodes(SceneNode* node, size_t& count) const;
        void TraverseNodes(SceneNode* node, const std::function<void(SceneNode*)>& visitor) const;
        void UpdateNodeRecursive(SceneNode* node, float deltaTime);
        void RemoveNodeRecursive(SceneNode* node);
        bool IsNodeInFrustum(const SceneNode& node, const Camera& camera) const;
        bool IsNodeInSphere(const SceneNode& node, const glm::vec3& center, float radius) const;
        bool IsNodeInBox(const SceneNode& node, const glm::vec3& min, const glm::vec3& max) const;
        void NotifyNodeAdded(std::shared_ptr<SceneNode> node);
        void NotifyNodeRemoved(std::shared_ptr<SceneNode> node);
    };

    // 为了向后兼容，保留Scene类型别名
    using Scene = SceneSystem;

} // namespace HybridPBR