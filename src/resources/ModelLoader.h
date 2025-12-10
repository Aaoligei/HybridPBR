#pragma once
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "../resources/Mesh.h"
#include "../rendering/common/Material.h"
#include "../rendering/common/Texture.h"  // 新增Texture头文件
#include "utils/Logger.h"

// Assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace HybridPBR {

    struct ModelLoadResult {
        bool success = false;
        std::string errorMessage;
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Material>> materials;
    };

    class ModelLoader {
    public:
        static ModelLoadResult LoadFromFile(const std::string& filepath);

    private:
        static void ProcessNode(aiNode* node, const aiScene* scene,
                               std::vector<std::shared_ptr<Mesh>>& meshes,
                               std::vector<std::shared_ptr<Material>>& materials,
                               const std::string& directory);
        
        static std::shared_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene);
        static std::shared_ptr<Material> ProcessMaterial(aiMaterial* material, const std::string& directory);

        // 新增的纹理加载函数
        static void LoadMaterialTextures(aiMaterial* aiMat, std::shared_ptr<Material> material, const std::string& directory);
        static std::shared_ptr<Texture> LoadTextureFromMaterial(aiMaterial* aiMat, const std::string& directory, 
                                                               aiTextureType aiType, TextureType type);
        
        // 辅助转换函数
        static glm::vec3 AssimpToGLM(aiVector3D vec);
        static glm::vec2 AssimpToGLM(aiVector2D vec);
        static glm::mat4 AssimpToGLM(aiMatrix4x4 mat);
    };


} // namespace HybridPBR