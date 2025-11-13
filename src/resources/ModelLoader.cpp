#include "ModelLoader.h"
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace HybridPBR {

    ModelLoadResult ModelLoader::LoadFromFile(const std::string& filepath) {
        ModelLoadResult result;
        
        // 检查文件是否存在
        if (!std::filesystem::exists(filepath)) {
            result.success = false;
            result.errorMessage = "File not found: " + filepath;
            return result;
        }
        
        // 获取文件目录
        std::filesystem::path path(filepath);
        std::string directory = path.parent_path().string();
        
        // 初始化Assimp导入器
        Assimp::Importer importer;
        
        // 导入模型
        const aiScene* scene = importer.ReadFile(filepath, 
            aiProcess_Triangulate | 
            aiProcess_FlipUVs | 
            aiProcess_GenNormals | 
            aiProcess_CalcTangentSpace);
        
        // 检查是否有错误
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            result.success = false;
            result.errorMessage = "Assimp error: " + std::string(importer.GetErrorString());
            LOG_ERROR(result.errorMessage);
            return result;
        }
        
        // 处理场景节点
        ProcessNode(scene->mRootNode, scene, result.meshes, result.materials, directory);
        
        result.success = true;
        LOG_INFO("Successfully loaded model: " + filepath);
        return result;
    }
    
    void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, 
                                 std::vector<std::shared_ptr<Mesh>>& meshes,
                                 std::vector<std::shared_ptr<Material>>& materials,
                                 const std::string& directory) {
        // 处理当前节点的所有网格
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(ProcessMesh(mesh, scene));
        }
        
        // 处理材质
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* material = scene->mMaterials[i];
            materials.push_back(ProcessMaterial(material, directory));
        }
        
        // 递归处理所有子节点
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene, meshes, materials, directory);
        }
    }
    
    std::shared_ptr<Mesh> ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        
        // 处理顶点
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            
            // 位置
            if (mesh->HasPositions()) {
                vertex.position = AssimpToGLM(mesh->mVertices[i]);
            }
            
            // 法线
            if (mesh->HasNormals()) {
                vertex.normal = AssimpToGLM(mesh->mNormals[i]);
            }
            
            // 纹理坐标
            if (mesh->mTextureCoords[0]) {
                vertex.texcoord = AssimpToGLM(mesh->mTextureCoords[0][i]);
            } else {
                vertex.texcoord = glm::vec2(0.0f, 0.0f);
            }
            
            // 切线和副切线
            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent = AssimpToGLM(mesh->mTangents[i]);
                vertex.bitangent = AssimpToGLM(mesh->mBitangents[i]);
            } else {
                vertex.tangent = glm::vec3(0.0f);
                vertex.bitangent = glm::vec3(0.0f);
            }
            
            vertices.push_back(vertex);
        }
        
        // 处理索引
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }
        
        // 创建网格对象
        auto resultMesh = std::make_shared<Mesh>(mesh->mName.C_Str());
        resultMesh->SetVertices(vertices);
        resultMesh->SetIndices(indices);
        
        return resultMesh;
    }
    
    std::shared_ptr<Material> ModelLoader::ProcessMaterial(aiMaterial* material, const std::string& directory) {
        // 获取材质名称
        aiString name;
        material->Get(AI_MATKEY_NAME, name);
        
        auto resultMaterial = std::make_shared<Material>(name.C_Str());
        auto& properties = resultMaterial->GetProperties();
        
        // 获取漫反射颜色
        aiColor3D color(0.f, 0.f, 0.f);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        properties.albedo = glm::vec4(color.r, color.g, color.b, 1.0f);
        
        // 获取金属度
        float metallic;
        if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, metallic)) {
            // Assimp中的光泽度需要转换为金属度
            properties.metallic = metallic / 100.0f;
        } else {
            properties.metallic = 0.0f;
        }
        
        // 获取粗糙度
        float roughness;
        if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, roughness)) {
            // 粗糙度通常与光泽度相反
            properties.roughness = 1.0f - (roughness / 100.0f);
        } else {
            properties.roughness = 0.5f;
        }
        
        // TODO: 加载纹理
        // 这里可以添加纹理加载逻辑
        
        return resultMaterial;
    }
    
    glm::vec3 ModelLoader::AssimpToGLM(aiVector3D vec) {
        return glm::vec3(vec.x, vec.y, vec.z);
    }
    
    glm::vec2 ModelLoader::AssimpToGLM(aiVector2D vec) {
        return glm::vec2(vec.x, vec.y);
    }
    
    glm::mat4 ModelLoader::AssimpToGLM(aiMatrix4x4 mat) {
        glm::mat4 result;
        // Assimp矩阵是行主序，GLM是列主序，需要转置
        result[0][0] = mat.a1; result[0][1] = mat.b1; result[0][2] = mat.c1; result[0][3] = mat.d1;
        result[1][0] = mat.a2; result[1][1] = mat.b2; result[1][2] = mat.c2; result[1][3] = mat.d2;
        result[2][0] = mat.a3; result[2][1] = mat.b3; result[2][2] = mat.c3; result[2][3] = mat.d3;
        result[3][0] = mat.a4; result[3][1] = mat.b4; result[3][2] = mat.c4; result[3][3] = mat.d4;
        return result;
    }

} // namespace HybridPBR