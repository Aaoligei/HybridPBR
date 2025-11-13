#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "../rendering/common/Texture.h"
#include "../rendering/common/Material.h"
#include "../rendering/rasterization/Mesh.h"
#include "../rendering/Shader.h"
#include "utils/Logger.h"

namespace HybridPBR {

    class ResourceManager {
    public:
        static ResourceManager& GetInstance();
        
        // 资源加载
        std::shared_ptr<Shader> LoadShader(const std::string& name, 
                                          const std::string& vertexPath, 
                                          const std::string& fragmentPath);
        std::shared_ptr<Texture> LoadTexture(const std::string& filepath, 
                                            TextureType type = TextureType::DIFFUSE);
        std::shared_ptr<Mesh> LoadMesh(const std::string& filepath);
        std::shared_ptr<Material> CreateMaterial(const std::string& name, 
                                                const MaterialProperties& properties);
        
        // 资源获取
        std::shared_ptr<Shader> GetShader(const std::string& name);
        std::shared_ptr<Texture> GetTexture(const std::string& filepath);
        std::shared_ptr<Mesh> GetMesh(const std::string& filepath);
        std::shared_ptr<Material> GetMaterial(const std::string& name);
        
        // 资源清理
        void ClearUnusedResources();
        void ClearAllResources();
        
        // 统计信息
        size_t GetShaderCount() const { return shaders.size(); }
        size_t GetTextureCount() const { return textures.size(); }
        size_t GetMeshCount() const { return meshes.size(); }
        size_t GetMaterialCount() const { return materials.size(); }

    private:
        ResourceManager() = default;
        
        std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
        std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
        std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
        std::unordered_map<std::string, std::shared_ptr<Material>> materials;
    };

} // namespace HybridPBR