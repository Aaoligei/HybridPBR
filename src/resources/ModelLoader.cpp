#include "ModelLoader.h"
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../rendering/common/Texture.h"

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
        
        // 导入模型 - 添加更多处理选项以确保UV正确
        const aiScene* scene = importer.ReadFile(filepath, 
            aiProcess_Triangulate | 
            aiProcess_FlipUVs | 
            aiProcess_GenNormals | 
            aiProcess_CalcTangentSpace |
            aiProcess_PreTransformVertices); // 添加预变换以确保正确处理
        
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
            
            // 纹理坐标 - 更安全的处理方式
            vertex.texcoord = glm::vec2(0.0f, 0.0f); // 默认值
            if (mesh->mTextureCoords[0] != nullptr && mesh->mNumUVComponents[0] >= 2) {
                // 确保我们有有效的纹理坐标和足够的组件
                vertex.texcoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                
                // 检查是否需要翻转V坐标（取决于纹理的坐标系统）
                // OpenGL的纹理坐标系统原点在左下角，而很多模型格式原点在左上角
                vertex.texcoord.y = 1.0f - vertex.texcoord.y;
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
        
        // 加载纹理
        LoadMaterialTextures(material, resultMaterial, directory);
        
        return resultMaterial;
    }
    
    void ModelLoader::LoadMaterialTextures(aiMaterial* aiMat, std::shared_ptr<Material> material, const std::string& directory) {
        // 加载漫反射纹理
        auto diffuseTexture = LoadTextureFromMaterial(aiMat, directory, aiTextureType_DIFFUSE, TextureType::DIFFUSE);
        if (diffuseTexture) {
            material->SetTexture(TextureType::DIFFUSE, diffuseTexture);
        }
        
        // 加载法线纹理
        auto normalTexture = LoadTextureFromMaterial(aiMat, directory, aiTextureType_NORMALS, TextureType::NORMAL);
        if (normalTexture) {
            material->SetTexture(TextureType::NORMAL, normalTexture);
        }
        
        // 加载金属度纹理
        auto metallicTexture = LoadTextureFromMaterial(aiMat, directory, aiTextureType_METALNESS, TextureType::METALLIC);
        if (metallicTexture) {
            material->SetTexture(TextureType::METALLIC, metallicTexture);
        }
        
        // 加载粗糙度纹理
        auto roughnessTexture = LoadTextureFromMaterial(aiMat, directory, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::ROUGHNESS);
        if (roughnessTexture) {
            material->SetTexture(TextureType::ROUGHNESS, roughnessTexture);
        }
        
        // 加载环境光遮蔽纹理
        auto aoTexture = LoadTextureFromMaterial(aiMat, directory, aiTextureType_AMBIENT_OCCLUSION, TextureType::AMBIENT_OCCLUSION);
        if (aoTexture) {
            material->SetTexture(TextureType::AMBIENT_OCCLUSION, aoTexture);
        }
        
        // 加载自发光纹理
        auto emissiveTexture = LoadTextureFromMaterial(aiMat, directory, aiTextureType_EMISSIVE, TextureType::EMISSIVE);
        if (emissiveTexture) {
            material->SetTexture(TextureType::EMISSIVE, emissiveTexture);
        }
    }
    
    std::shared_ptr<Texture> ModelLoader::LoadTextureFromMaterial(aiMaterial* aiMat, const std::string& directory, 
                                                                 aiTextureType aiType, TextureType type) {
        // 检查是否有这种类型的纹理
        if (aiMat->GetTextureCount(aiType) == 0) {
            return nullptr;
        }
        
        // 获取第一个纹理
        aiString str;
        if (aiMat->GetTexture(aiType, 0, &str) != AI_SUCCESS) {
            return nullptr;
        }
        
        // 处理纹理路径
        std::string filename = str.C_Str();
        std::string filepath;
        
        // 如果是相对路径，则拼接目录
        if (filename[0] == '.' || filename[0] == '/' || filename[0] == '\\') {
            filepath = directory + "/" + filename;
        } else {
            // 检查文件是否在模型目录中
            filepath = directory + "/" + filename;
            if (!std::filesystem::exists(filepath)) {
                filepath = filename; // 使用原始路径
            }
        }
        
        // 检查文件是否存在
        if (!std::filesystem::exists(filepath)) {
            LOG_WARNING("Texture file not found: " + filepath);
            return nullptr;
        }
        
        // 创建并加载纹理
        auto texture = std::make_shared<Texture>();
        if (texture->LoadFromFile(filepath, type)) {
            LOG_INFO("Successfully loaded texture: " + filepath);
            return texture;
        } else {
            LOG_WARNING("Failed to load texture: " + filepath);
            return nullptr;
        }
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