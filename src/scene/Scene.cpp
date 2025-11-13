#include "Scene.h"
#include "../utils/Logger.h"
#include <fstream>
#include <nlohmann/json.hpp>

// 使用更简洁的别名
using nlohmann::json;

namespace HybridPBR {

    Scene::Scene() {
        root = std::make_shared<SceneNode>("Root");
        BuildNodeMap(root.get());
        // 初始化时计算节点数量
        nodeCount = 1; // 至少包括根节点
        for (const auto& pair : nodeMap) {
            if (pair.second.get() != root.get()) {
                nodeCount++;
            }
        }
    }

    Scene::~Scene() {
        // 自动清理所有资源
        lights.clear();
        nodeMap.clear();
    }

    std::shared_ptr<SceneNode> Scene::CreateNode(const std::string& name) {
        auto node = std::make_shared<SceneNode>(name);
        AddNode(node);
        return node;
    }

    void Scene::AddNode(std::shared_ptr<SceneNode> node) {
        if (!node) return;
        
        root->AddChild(node);
        BuildNodeMap(root.get()); // 重建节点映射
        // 更新节点计数
        nodeCount = 1; // 重新计算节点数量，包括根节点
        for (const auto& pair : nodeMap) {
            if (pair.second.get() != root.get()) {
                nodeCount++;
            }
        }
    }

    void Scene::RemoveNode(SceneNode* node) {
        if (!node) return;
        
        root->RemoveChild(node);
        BuildNodeMap(root.get()); // 重建节点映射
        // 更新节点计数
        nodeCount = 1; // 重新计算节点数量，包括根节点
        for (const auto& pair : nodeMap) {
            if (pair.second.get() != root.get()) {
                nodeCount++;
            }
        }
    }

    std::shared_ptr<SceneNode> Scene::FindNode(const std::string& name) {
        auto it = nodeMap.find(name);
        if (it != nodeMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    void Scene::SetMainCamera(std::shared_ptr<Camera> camera) {
        mainCamera = camera;
    }

    void Scene::AddLight(std::shared_ptr<Light> light) {
        if (!light) return;
        
        lights.push_back(light);
        LOG_INFO("Added light to scene: " + light->GetName());
    }

    void Scene::RemoveLight(Light* lightToRemove) {
        for (auto it = lights.begin(); it != lights.end(); ++it) {
            if (it->get() == lightToRemove) {
                lights.erase(it);
                LOG_INFO("Removed light from scene");
                break;
            }
        }
    }

    void Scene::Update() {
        root->Update();
    }

    bool Scene::SaveToFile(const std::string& filepath) {
        try {
            json sceneJson;
            
            // 保存根节点及其子节点
            json rootNodeJson;
            rootNodeJson["name"] = root->GetName();
            
            // 保存根节点的变换信息
            auto& rootTransform = root->GetTransform();
            json rootTransformJson;
            rootTransformJson["position"] = {rootTransform.GetPosition().x, rootTransform.GetPosition().y, rootTransform.GetPosition().z};
            rootTransformJson["rotation"] = {rootTransform.GetRotation().x, rootTransform.GetRotation().y, rootTransform.GetRotation().z};
            rootTransformJson["scale"] = {rootTransform.GetScale().x, rootTransform.GetScale().y, rootTransform.GetScale().z};
            rootNodeJson["transform"] = rootTransformJson;
            
            // 递归保存子节点
            json childrenJson = json::array();
            for (const auto& child : root->GetChildren()) {
                json childJson;
                childJson["name"] = child->GetName();
                
                auto& transform = child->GetTransform();
                json transformJson;
                transformJson["position"] = {transform.GetPosition().x, transform.GetPosition().y, transform.GetPosition().z};
                transformJson["rotation"] = {transform.GetRotation().x, transform.GetRotation().y, transform.GetRotation().z};
                transformJson["scale"] = {transform.GetScale().x, transform.GetScale().y, transform.GetScale().z};
                childJson["transform"] = transformJson;
                
                childrenJson.push_back(childJson);
            }
            rootNodeJson["children"] = childrenJson;
            sceneJson["root"] = rootNodeJson;
            
            // 保存相机信息
            if (mainCamera) {
                json cameraJson;
                cameraJson["position"] = {mainCamera->GetPosition().x, mainCamera->GetPosition().y, mainCamera->GetPosition().z};
                cameraJson["fov"] = mainCamera->GetFOV();
                cameraJson["nearPlane"] = mainCamera->GetNearPlane();
                cameraJson["farPlane"] = mainCamera->GetFarPlane();
                sceneJson["camera"] = cameraJson;
            }
            
            // 保存光源信息
            json lightsJson = json::array();
            for (const auto& light : lights) {
                json lightJson;
                lightJson["name"] = light->GetName();
                lightJson["type"] = static_cast<int>(light->GetType());
                lightJson["position"] = {light->GetPosition().x, light->GetPosition().y, light->GetPosition().z};
                lightJson["enabled"] = light->IsEnabled();
                
                auto& properties = light->GetProperties();
                json propertiesJson;
                propertiesJson["color"] = {properties.color.x, properties.color.y, properties.color.z};
                propertiesJson["intensity"] = properties.intensity;
                propertiesJson["range"] = properties.range;
                propertiesJson["constant"] = properties.constant;
                propertiesJson["linear"] = properties.linear;
                propertiesJson["quadratic"] = properties.quadratic;
                propertiesJson["innerCutoff"] = properties.innerCutoff;
                propertiesJson["outerCutoff"] = properties.outerCutoff;
                
                lightJson["properties"] = propertiesJson;
                lightsJson.push_back(lightJson);
            }
            sceneJson["lights"] = lightsJson;
            
            // 写入文件
            std::ofstream file(filepath);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open file for writing: " + filepath);
                return false;
            }
            
            file << sceneJson.dump(4);
            file.close();
            
            LOG_INFO("Scene successfully saved to: " + filepath);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to save scene to file: " + std::string(e.what()));
            return false;
        }
    }

    bool Scene::LoadFromFile(const std::string& filepath) {
        try {
            // 读取文件
            std::ifstream file(filepath);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open file for reading: " + filepath);
                return false;
            }
            
            json sceneJson;
            file >> sceneJson;
            file.close();
            
            // 清空当前场景
            root = std::make_shared<SceneNode>("Root");
            lights.clear();
            
            // 加载根节点
            if (sceneJson.contains("root")) {
                auto& rootNodeJson = sceneJson["root"];
                if (rootNodeJson.contains("transform")) {
                    auto& rootTransformJson = rootNodeJson["transform"];
                    auto& rootTransform = root->GetTransform();
                    
                    if (rootTransformJson.contains("position")) {
                        auto pos = rootTransformJson["position"];
                        rootTransform.SetPosition(glm::vec3(pos[0], pos[1], pos[2]));
                    }
                    
                    if (rootTransformJson.contains("rotation")) {
                        auto rot = rootTransformJson["rotation"];
                        rootTransform.SetRotation(glm::vec3(rot[0], rot[1], rot[2]));
                    }
                    
                    if (rootTransformJson.contains("scale")) {
                        auto scale = rootTransformJson["scale"];
                        rootTransform.SetScale(glm::vec3(scale[0], scale[1], scale[2]));
                    }
                }
                
                // 加载子节点
                if (rootNodeJson.contains("children")) {
                    for (const auto& childJson : rootNodeJson["children"]) {
                        std::string name = childJson.value("name", "Node");
                        auto node = std::make_shared<SceneNode>(name);
                        
                        if (childJson.contains("transform")) {
                            auto& transformJson = childJson["transform"];
                            auto& transform = node->GetTransform();
                            
                            if (transformJson.contains("position")) {
                                auto pos = transformJson["position"];
                                transform.SetPosition(glm::vec3(pos[0], pos[1], pos[2]));
                            }
                            
                            if (transformJson.contains("rotation")) {
                                auto rot = transformJson["rotation"];
                                transform.SetRotation(glm::vec3(rot[0], rot[1], rot[2]));
                            }
                            
                            if (transformJson.contains("scale")) {
                                auto scale = transformJson["scale"];
                                transform.SetScale(glm::vec3(scale[0], scale[1], scale[2]));
                            }
                        }
                        
                        root->AddChild(node);
                    }
                }
            }
            
            // 加载相机
            if (sceneJson.contains("camera")) {
                auto& cameraJson = sceneJson["camera"];
                mainCamera = std::make_shared<Camera>();
                
                if (cameraJson.contains("position")) {
                    auto pos = cameraJson["position"];
                    mainCamera->SetPosition(glm::vec3(pos[0], pos[1], pos[2]));
                }
                
                float fov = cameraJson.value("fov", 45.0f);
                float nearPlane = cameraJson.value("nearPlane", 0.1f);
                float farPlane = cameraJson.value("farPlane", 100.0f);
                // 注意：这里缺少aspect参数，暂时使用默认值
                mainCamera->SetPerspective(fov, 16.0f/9.0f, nearPlane, farPlane);
            }
            
            // 加载光源
            if (sceneJson.contains("lights")) {
                for (const auto& lightJson : sceneJson["lights"]) {
                    std::string name = lightJson.value("name", "Light");
                    int type = lightJson.value("type", 0);
                    bool enabled = lightJson.value("enabled", true);
                    
                    auto light = std::make_shared<Light>(static_cast<LightType>(type), name);
                    light->SetEnabled(enabled);
                    
                    if (lightJson.contains("position")) {
                        auto pos = lightJson["position"];
                        light->SetPosition(glm::vec3(pos[0], pos[1], pos[2]));
                    }
                    
                    if (lightJson.contains("properties")) {
                        auto& propertiesJson = lightJson["properties"];
                        auto& properties = light->GetProperties();
                        
                        if (propertiesJson.contains("color")) {
                            auto color = propertiesJson["color"];
                            properties.color = glm::vec3(color[0], color[1], color[2]);
                        }
                        
                        properties.intensity = propertiesJson.value("intensity", 1.0f);
                        properties.range = propertiesJson.value("range", 10.0f);
                        properties.constant = propertiesJson.value("constant", 1.0f);
                        properties.linear = propertiesJson.value("linear", 0.09f);
                        properties.quadratic = propertiesJson.value("quadratic", 0.032f);
                        properties.innerCutoff = propertiesJson.value("innerCutoff", glm::cos(glm::radians(12.5f)));
                        properties.outerCutoff = propertiesJson.value("outerCutoff", glm::cos(glm::radians(17.5f)));
                    }
                    
                    lights.push_back(light);
                }
            }
            
            // 重建节点映射和节点计数
            BuildNodeMap(root.get());
            
            LOG_INFO("Scene successfully loaded from: " + filepath);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to load scene from file: " + std::string(e.what()));
            return false;
        }
    }

    void Scene::BuildNodeMap(SceneNode* node) {
        if (!node) return;
        
        nodeMap[node->GetName()] = std::shared_ptr<SceneNode>(node, [](SceneNode*) {}); // 空删除器
        
        for (auto& child : node->GetChildren()) {
            BuildNodeMap(child.get());
        }
    }


} // namespace HybridPBR