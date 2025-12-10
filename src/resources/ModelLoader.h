#pragma once
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "../resources/Mesh.h"
#include "../rendering/common/Material.h"
#include "../rendering/common/Texture.h"  // 新增Texture头文件
#include "utils/Logger.h"
#include "../rhi/RHI_Device.h"

// Assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace HybridPBR {
    // [新增] 简单的纹理缓存，防止重复加载同一张贴图
    struct TextureCache {
        std::unordered_map<std::string, TextureHandle> loadedTextures;
    };
    struct ModelLoadResult {
        bool success = false;
        std::string errorMessage;
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Material>> materials;
    };

    class ModelLoader {
    public:
        static ModelLoadResult LoadFromFile(const std::string& filepath, RHI_Device* device);

    private:
        static void ProcessNode(aiNode* node, const aiScene* scene,
                               std::vector<std::shared_ptr<Mesh>>& meshes,
                               std::vector<std::shared_ptr<Material>>& materials,
                               const std::string& directory,
                               RHI_Device* device,
                               TextureCache& cache);
        
        static std::shared_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene);
        static std::shared_ptr<Material> ProcessMaterial(aiMaterial* material, 
                                                        const std::string& directory,
                                                        RHI_Device* device,
                                                        TextureCache& cache);

        // 新增的纹理加载函数
        static void LoadMaterialTextures(aiMaterial* aiMat, 
                                        std::shared_ptr<Material> material,
                                        const std::string& directory,
                                        RHI_Device* device,
                                        TextureCache& cache);
        
        // 辅助转换函数
        static glm::vec3 AssimpToGLM(aiVector3D vec);
        static glm::vec2 AssimpToGLM(aiVector2D vec);
        static glm::mat4 AssimpToGLM(aiMatrix4x4 mat);
    };


} // namespace HybridPBR