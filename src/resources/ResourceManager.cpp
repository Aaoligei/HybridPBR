#include "ResourceManager.h"
#include "ModelLoader.h"

namespace HybridPBR {

    ResourceManager& ResourceManager::GetInstance() {
        static ResourceManager instance;
        return instance;
    }

    std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string& name, 
                                                       const std::string& vertexPath, 
                                                       const std::string& fragmentPath) {
        auto it = shaders.find(name);
        if (it != shaders.end()) {
            return it->second;
        }

        auto shader = std::make_shared<Shader>();
        if (shader->LoadFromFile(vertexPath, fragmentPath)) {
            shaders[name] = shader;
            LOG_INFO("Loaded shader: " + name);
            return shader;
        }

        LOG_ERROR("Failed to load shader: " + name);
        return nullptr;
    }

    std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::string& filepath, 
                                                         TextureType type) {
        auto it = textures.find(filepath);
        if (it != textures.end()) {
            return it->second;
        }

        auto texture = std::make_shared<Texture>();
        if (texture->LoadFromFile(filepath, type)) {
            textures[filepath] = texture;
            LOG_INFO("Loaded texture: " + filepath);
            return texture;
        }

        LOG_ERROR("Failed to load texture: " + filepath);
        return nullptr;
    }

    std::shared_ptr<Mesh> ResourceManager::LoadMesh(const std::string& filepath) {
        auto it = meshes.find(filepath);
        if (it != meshes.end()) {
            return it->second;
        }

        auto result = ModelLoader::LoadFromFile(filepath);
        if (result.success && !result.meshes.empty()) {
            auto mesh = result.meshes[0];
            meshes[filepath] = mesh;
            LOG_INFO("Loaded mesh: " + filepath);
            return mesh;
        }

        LOG_ERROR("Failed to load mesh: " + filepath);
        return nullptr;
    }

    std::shared_ptr<Material> ResourceManager::CreateMaterial(const std::string& name, 
                                                             const MaterialProperties& properties) {
        auto it = materials.find(name);
        if (it != materials.end()) {
            return it->second;
        }

        auto material = std::make_shared<Material>(name, properties);
        materials[name] = material;
        LOG_INFO("Created material: " + name);
        return material;
    }

    std::shared_ptr<Shader> ResourceManager::GetShader(const std::string& name) {
        auto it = shaders.find(name);
        return it != shaders.end() ? it->second : nullptr;
    }

    std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& filepath) {
        auto it = textures.find(filepath);
        return it != textures.end() ? it->second : nullptr;
    }

    std::shared_ptr<Mesh> ResourceManager::GetMesh(const std::string& filepath) {
        auto it = meshes.find(filepath);
        return it != meshes.end() ? it->second : nullptr;
    }

    std::shared_ptr<Material> ResourceManager::GetMaterial(const std::string& name) {
        auto it = materials.find(name);
        return it != materials.end() ? it->second : nullptr;
    }

    void ResourceManager::ClearUnusedResources() {
        // 清理未被使用的着色器
        for (auto it = shaders.begin(); it != shaders.end(); ) {
            if (it->second.use_count() == 1) {
                it = shaders.erase(it);
            } else {
                ++it;
            }
        }

        // 清理未被使用的纹理
        for (auto it = textures.begin(); it != textures.end(); ) {
            if (it->second.use_count() == 1) {
                it = textures.erase(it);
            } else {
                ++it;
            }
        }

        // 清理未被使用的网格
        for (auto it = meshes.begin(); it != meshes.end(); ) {
            if (it->second.use_count() == 1) {
                it = meshes.erase(it);
            } else {
                ++it;
            }
        }

        // 清理未被使用的材质
        for (auto it = materials.begin(); it != materials.end(); ) {
            if (it->second.use_count() == 1) {
                it = materials.erase(it);
            } else {
                ++it;
            }
        }

        LOG_INFO("Cleared unused resources");
    }

    void ResourceManager::ClearAllResources() {
        shaders.clear();
        textures.clear();
        meshes.clear();
        materials.clear();
        LOG_INFO("Cleared all resources");
    }

} // namespace HybridPBR