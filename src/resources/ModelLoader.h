#pragma once
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "../rendering/rasterization/Mesh.h"
#include "../rendering/common/Material.h"
#include "utils/Logger.h"

// Assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace HybridPBR {

    struct ModelLoadResult {
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Material>> materials;
        bool success = false;
        std::string errorMessage;
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
        static glm::vec3 AssimpToGLM(aiVector3D vec);
        static glm::vec2 AssimpToGLM(aiVector2D vec);
        static glm::mat4 AssimpToGLM(aiMatrix4x4 mat);
    };

} // namespace HybridPBR