#include "Scene.h"
#include "utils/Logger.h"
#include <algorithm>
#include <sstream>

namespace HybridPBR {

    SceneSystem::SceneSystem() : nodeCount_(0), sceneDirty_(true), initialized_(false) {
    }

    SceneSystem::~SceneSystem() {
        Shutdown();
    }

    Result<void> SceneSystem::Initialize() {
        if (initialized_) {
            return Result<void>::Success();
        }

        LOG_INFO("SceneSystem", "Initializing scene system");

        // 创建根节点
        root_ = std::make_shared<SceneNode>("Root");
        nodeMap_["Root"] = root_;
        nodeCount_ = 1;

        initialized_ = true;
        LOG_INFO("SceneSystem", "Scene system initialized successfully");

        return Result<void>::Success();
    }

    void SceneSystem::Shutdown() {
        if (!initialized_) {
            return;
        }

        LOG_INFO("SceneSystem", "Shutting down scene system");

        // 清理所有节点
        root_.reset();
        nodeMap_.clear();
        lights_.clear();
        lightMap_.clear();
        mainCamera_.reset();

        nodeCount_ = 0;
        sceneDirty_ = true;
        initialized_ = false;

        LOG_INFO("SceneSystem", "Scene system shutdown");
    }

    void SceneSystem::Update(float deltaTime) {
        if (!initialized_) {
            return;
        }

        // 更新场景图
        if (root_) {
            UpdateNodeRecursive(root_.get(), deltaTime);
        }

        // 清理脏标记
        sceneDirty_ = false;
    }

    Result<std::shared_ptr<SceneNode>> SceneSystem::CreateNode(const std::string& name) {
        if (!initialized_) {
            return Result<std::shared_ptr<SceneNode>>::Failure(
                Error(ErrorType::Initialization, "Scene system not initialized"));
        }

        std::string nodeName = name;
        if (nodeName.empty()) {
            nodeName = "Node_" + std::to_string(nodeCount_);
        }

        // 确保名称唯一
        int counter = 1;
        std::string uniqueName = nodeName;
        while (nodeMap_.find(uniqueName) != nodeMap_.end()) {
            uniqueName = nodeName + "_" + std::to_string(counter++);
        }

        auto node = std::make_shared<SceneNode>(uniqueName);
        
        // 添加到场景图
        if (root_) {
            root_->AddChild(node);
        }

        // 更新映射
        nodeMap_[uniqueName] = node;
        nodeCount_++;
        sceneDirty_ = true;

        // 通知事件
        NotifyNodeAdded(node);

        LOG_DEBUG("SceneSystem", "Created scene node: " + uniqueName);

        return Result<std::shared_ptr<SceneNode>>::Success(node);
    }

    Result<void> SceneSystem::AddNode(std::shared_ptr<SceneNode> node) {
        if (!initialized_) {
            return Result<void>::Failure(
                Error(ErrorType::Initialization, "Scene system not initialized"));
        }

        if (!node) {
            return Result<void>::Failure(
                Error(ErrorType::InvalidParameter, "Invalid node"));
        }

        const std::string& name = node->GetName();
        
        // 检查名称冲突
        if (nodeMap_.find(name) != nodeMap_.end()) {
            return Result<void>::Failure(
                Error(ErrorType::InvalidParameter, "Node already exists: " + name));
        }

        // 添加到场景图
        if (root_) {
            root_->AddChild(node);
        }

        // 更新映射
        nodeMap_[name] = node;
        BuildNodeMap(node.get());
        nodeCount_++;
        sceneDirty_ = true;

        // 通知事件
        NotifyNodeAdded(node);

        LOG_DEBUG("SceneSystem", "Added scene node: " + name);

        return Result<void>::Success();
    }

    Result<void> SceneSystem::RemoveNode(const std::string& name) {
        if (!initialized_) {
            return Result<void>::Failure(
                Error(ErrorType::Initialization, "Scene system not initialized"));
        }

        auto it = nodeMap_.find(name);
        if (it == nodeMap_.end()) {
            return Result<void>::Failure(
                Error(ErrorType::InvalidParameter, "Node not found: " + name));
        }

        auto node = it->second;
        
        // 从父节点移除
        if (auto parent = node->GetParent()) {
            parent->RemoveChild(node.get());
        }

        // 递归移除所有子节点
        RemoveNodeRecursive(node.get());

        // 通知事件
        NotifyNodeRemoved(node);

        LOG_DEBUG("SceneSystem", "Removed scene node: " + name);

        return Result<void>::Success();
    }

    std::shared_ptr<SceneNode> SceneSystem::FindNode(const std::string& name) {
        auto it = nodeMap_.find(name);
        return (it != nodeMap_.end()) ? it->second : nullptr;
    }

    void SceneSystem::SetMainCamera(std::shared_ptr<Camera> camera) {
        mainCamera_ = camera;
        sceneDirty_ = true;
        
        if (camera) {
            LOG_DEBUG("SceneSystem", "Set main camera");
        } else {
            LOG_DEBUG("SceneSystem", "Cleared main camera");
        }
    }

    std::vector<std::shared_ptr<Camera>> SceneSystem::GetAllCameras() const {
        std::vector<std::shared_ptr<Camera>> cameras;
        
        if (mainCamera_) {
            cameras.push_back(mainCamera_);
        }

        // Note: Cameras are managed separately in SceneSystem, not as components of SceneNodes
        // In the current implementation, cameras are not stored in scene nodes

        return cameras;
    }

    Result<void> SceneSystem::AddLight(std::shared_ptr<Light> light) {
        if (!light) {
            return Result<void>::Failure(
                Error(ErrorType::InvalidParameter, "Invalid light"));
        }

        const std::string& name = light->GetName();
        
        // 检查名称冲突
        if (lightMap_.find(name) != lightMap_.end()) {
            return Result<void>::Failure(
                Error(ErrorType::InvalidParameter, "Light already exists: " + name));
        }

        lights_.push_back(light);
        lightMap_[name] = light;
        sceneDirty_ = true;

        LOG_DEBUG("SceneSystem", "Added light: " + name);

        return Result<void>::Success();
    }

    Result<void> SceneSystem::RemoveLight(const std::string& name) {
        auto it = lightMap_.find(name);
        if (it == lightMap_.end()) {
            return Result<void>::Failure(
                Error(ErrorType::InvalidParameter, "Light not found: " + name));
        }

        auto light = it->second;
        
        // 从列表中移除
        lights_.erase(std::remove(lights_.begin(), lights_.end(), light), lights_.end());
        lightMap_.erase(it);
        sceneDirty_ = true;

        LOG_DEBUG("SceneSystem", "Removed light: " + name);

        return Result<void>::Success();
    }

    std::shared_ptr<Light> SceneSystem::GetLight(const std::string& name) const {
        auto it = lightMap_.find(name);
        return (it != lightMap_.end()) ? it->second : nullptr;
    }

    std::vector<std::shared_ptr<Light>> SceneSystem::GetVisibleLights(const Camera& camera) const {
        std::vector<std::shared_ptr<Light>> visibleLights;
        
        for (const auto& light : lights_) {
            // 简化的可见性检查
            // 实际实现应该考虑光照范围和视锥体裁剪
            if (light->GetType() == LightType::DIRECTIONAL) {
                visibleLights.push_back(light);
            } else {
                // 点光源和聚光灯的距离检查
                float distance = glm::length(light->GetPosition() - camera.GetPosition());
                if (distance < light->GetProperties().range) {
                    visibleLights.push_back(light);
                }
            }
        }

        return visibleLights;
    }

    std::vector<std::shared_ptr<SceneNode>> SceneSystem::QueryNodes(const std::function<bool(const SceneNode&)>& predicate) const {
        std::vector<std::shared_ptr<SceneNode>> result;
        
        if (root_) {
            TraverseNodes(root_.get(), [&result, &predicate](SceneNode* node) {
                if (predicate(*node)) {
                    result.push_back(node->shared_from_this());
                }
            });
        }

        return result;
    }

    std::vector<std::shared_ptr<SceneNode>> SceneSystem::GetNodesInFrustum(const Camera& camera) const {
        return QueryNodes([&camera, this](const SceneNode& node) {
            return IsNodeInFrustum(node, camera);
        });
    }

    std::vector<std::shared_ptr<SceneNode>> SceneSystem::GetNodesInSphere(const glm::vec3& center, float radius) const {
        return QueryNodes([&center, radius, this](const SceneNode& node) {
            return IsNodeInSphere(node, center, radius);
        });
    }

    std::vector<std::shared_ptr<SceneNode>> SceneSystem::GetNodesInBox(const glm::vec3& min, const glm::vec3& max) const {
        return QueryNodes([&min, &max, this](const SceneNode& node) {
            return IsNodeInBox(node, min, max);
        });
    }

    Result<void> SceneSystem::SaveToFile(const std::string& filepath) {
        // 简化的序列化实现
        LOG_INFO("SceneSystem", "Saving scene to file: " + filepath);
        
        // TODO: 实现完整的场景序列化
        
        return Result<void>::Success();
    }

    Result<void> SceneSystem::LoadFromFile(const std::string& filepath) {
        // 简化的反序列化实现
        LOG_INFO("SceneSystem", "Loading scene from file: " + filepath);
        
        // TODO: 实现完整的场景反序列化
        
        sceneDirty_ = true;
        
        return Result<void>::Success();
    }

    // 私有方法实现
    void SceneSystem::BuildNodeMap(SceneNode* node) {
        if (!node) return;
        
        nodeMap_[node->GetName()] = node->shared_from_this();
        nodeCount_++;
        
        for (auto& child : node->GetChildren()) {
            BuildNodeMap(child.get());
        }
    }

    void SceneSystem::CountNodes(SceneNode* node, size_t& count) const {
        if (!node) return;
        
        count++;
        
        for (auto& child : node->GetChildren()) {
            CountNodes(child.get(), count);
        }
    }

    void SceneSystem::TraverseNodes(SceneNode* node, const std::function<void(SceneNode*)>& visitor) const {
        if (!node) return;
        
        visitor(node);
        
        for (auto& child : node->GetChildren()) {
            TraverseNodes(child.get(), visitor);
        }
    }

    void SceneSystem::UpdateNodeRecursive(SceneNode* node, float deltaTime) {
        if (!node) return;
        
        // 更新节点
        node->Update();
        
        // 更新子节点
        for (auto& child : node->GetChildren()) {
            UpdateNodeRecursive(child.get(), deltaTime);
        }
    }

    void SceneSystem::RemoveNodeRecursive(SceneNode* node) {
        if (!node) return;
        
        // 递归移除子节点
        for (auto& child : node->GetChildren()) {
            RemoveNodeRecursive(child.get());
        }
        
        // 从映射中移除
        nodeMap_.erase(node->GetName());
        nodeCount_--;
    }

    bool SceneSystem::IsNodeInFrustum(const SceneNode& node, const Camera& camera) const {
        // 简化的视锥体检查
        // 实际实现应该使用节点的包围盒
        glm::vec3 position = node.GetTransform().GetPosition();
        
        // 将世界坐标转换到视图空间
        glm::mat4 viewMatrix = camera.GetViewMatrix();
        glm::vec4 viewPos = viewMatrix * glm::vec4(position, 1.0f);
        
        // 检查是否在视锥体内
        glm::mat4 projectionMatrix = camera.GetProjectionMatrix();
        glm::vec4 clipPos = projectionMatrix * viewPos;
        
        return (clipPos.x >= -clipPos.w && clipPos.x <= clipPos.w &&
                clipPos.y >= -clipPos.w && clipPos.y <= clipPos.w &&
                clipPos.z >= -clipPos.w && clipPos.z <= clipPos.w);
    }

    bool SceneSystem::IsNodeInSphere(const SceneNode& node, const glm::vec3& center, float radius) const {
        glm::vec3 position = node.GetTransform().GetPosition();
        float distance = glm::length(position - center);
        return distance <= radius;
    }

    bool SceneSystem::IsNodeInBox(const SceneNode& node, const glm::vec3& min, const glm::vec3& max) const {
        glm::vec3 position = node.GetTransform().GetPosition();
        return (position.x >= min.x && position.x <= max.x &&
                position.y >= min.y && position.y <= max.y &&
                position.z >= min.z && position.z <= max.z);
    }

    void SceneSystem::NotifyNodeAdded(std::shared_ptr<SceneNode> node) {
        for (auto& callback : nodeAddedCallbacks_) {
            callback(node);
        }
    }

    void SceneSystem::NotifyNodeRemoved(std::shared_ptr<SceneNode> node) {
        for (auto& callback : nodeRemovedCallbacks_) {
            callback(node);
        }
    }

} // namespace HybridPBR