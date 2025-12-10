#include "ModelLoader.h"
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm> // for std::replace
#include "utils/Logger.h"
#include "utils/FileIO.h"

namespace HybridPBR {

// 辅助函数：标准化路径分隔符
    std::string NormalizePath(const std::string& path) {
        std::string p = path;
        std::replace(p.begin(), p.end(), '\\', '/');
        return p;
    }

    // 辅助：加载纹理并创建 RHI 资源
    TextureHandle LoadTextureRHI(const std::string& rawPath, RHI_Device* device, bool sRGB) {
        if (!device) return TextureHandle::Invalid();

        std::string path = NormalizePath(rawPath);

        // 1. 使用 FileIO 读取二进制数据 (避开 stbi_load 的路径坑)
        std::vector<char> fileData = FileIO::ReadBinaryFile(path);
        
        if (fileData.empty()) {
            LOG_ERROR("Texture file missing or empty: " + path);
            return TextureHandle::Invalid();
        }

        int width, height, channels;
        
        // 2. [核心修复] 调用 Texture 类的静态方法进行解码
        // 这会使用 Texture.cpp 中那个隔离了符号的 stbi_load_from_memory
        unsigned char* data = Texture::LoadImageFromMemory(
            fileData.data(), 
            static_cast<int>(fileData.size()), 
            &width, &height, &channels, 4);
        
        if (!data) {
            LOG_ERROR("STB Decode Failed: " + path + " (Likely Assimp conflict resolved now)");
            return TextureHandle::Invalid();
        }

        TextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = sRGB ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8_UNORM; 
        desc.name = std::filesystem::path(path).filename().string();

        TextureHandle handle = device->CreateTexture(desc, data);
        
        if (handle.IsValid()) {
            device->GenerateMipmaps(handle);
        }

        // 3. 释放内存
        Texture::FreeImage(data);
        return handle;
    }

    ModelLoadResult ModelLoader::LoadFromFile(const std::string& filepath, RHI_Device* device) {
        ModelLoadResult result;
        TextureCache textureCache;

        std::string normPath = NormalizePath(filepath);

        // 检查文件
        if (!std::filesystem::exists(normPath)) {
            result.success = false;
            result.errorMessage = "File not found: " + normPath;
            return result;
        }
        
        std::filesystem::path pathObj(normPath);
        std::string directory = pathObj.parent_path().string();
        
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(normPath, 
            aiProcess_Triangulate | 
            aiProcess_FlipUVs | 
            aiProcess_GenNormals | 
            aiProcess_CalcTangentSpace |
            aiProcess_PreTransformVertices);
        
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            result.success = false;
            result.errorMessage = "Assimp error: " + std::string(importer.GetErrorString());
            LOG_ERROR(result.errorMessage);
            return result;
        }
        
        ProcessNode(scene->mRootNode, scene, result.meshes, result.materials, directory, device, textureCache);
        
        result.success = true;
        LOG_INFO("Successfully loaded model: " + normPath);
        return result;
    }
    
    void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, 
                                 std::vector<std::shared_ptr<Mesh>>& meshes,
                                 std::vector<std::shared_ptr<Material>>& materials,
                                 const std::string& directory,
                                 RHI_Device* device,
                                 TextureCache& cache) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(ProcessMesh(mesh, scene));
        }
        
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* material = scene->mMaterials[i];
            materials.push_back(ProcessMaterial(material, directory, device, cache));
        }
        
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene, meshes, materials, directory, device, cache);
        }
    }
    
    std::shared_ptr<Mesh> ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            if (mesh->HasPositions()) vertex.position = AssimpToGLM(mesh->mVertices[i]);
            if (mesh->HasNormals()) vertex.normal = AssimpToGLM(mesh->mNormals[i]);
            
            vertex.texcoord = glm::vec2(0.0f, 0.0f);
            if (mesh->mTextureCoords[0] != nullptr && mesh->mNumUVComponents[0] >= 2) {
                vertex.texcoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                // GLTF 贴图通常不需要翻转，但如果之前的代码翻转了，这里保持一致
                // 如果发现贴图是倒的，把下面这行注释掉即可
                vertex.texcoord.y = 1.0f - vertex.texcoord.y; 
            }
            
            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent = AssimpToGLM(mesh->mTangents[i]);
                vertex.bitangent = AssimpToGLM(mesh->mBitangents[i]);
            } else {
                vertex.tangent = glm::vec3(0.0f);
                vertex.bitangent = glm::vec3(0.0f);
            }
            vertices.push_back(vertex);
        }
        
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) indices.push_back(face.mIndices[j]);
        }
        
        auto resultMesh = std::make_shared<Mesh>(mesh->mName.C_Str());
        resultMesh->SetVertices(vertices);
        resultMesh->SetIndices(indices);
        
        // 如果 Assimp 没算切线，我们自己算
        if (!vertices.empty() && vertices[0].tangent == glm::vec3(0.0f)) {
            resultMesh->CalculateTangents();
        }
        
        return resultMesh;
    }
    
    std::shared_ptr<Material> ModelLoader::ProcessMaterial(aiMaterial* material, 
                                                         const std::string& directory,
                                                         RHI_Device* device,
                                                         TextureCache& cache) {
        aiString name;
        material->Get(AI_MATKEY_NAME, name);
        
        // 1. 创建材质
        auto resultMaterial = std::make_shared<Material>(name.C_Str());
        
        // 2. 初始化 UBO
        if (device) resultMaterial->Initialize(device);

        // 3. 读取属性
        aiColor3D color(0.f, 0.f, 0.f);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        resultMaterial->SetAlbedoColor(glm::vec4(color.r, color.g, color.b, 1.0f));
        
        float metallic = 0.0f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, metallic)) {
             resultMaterial->SetMetallic(metallic / 100.0f);
        }

        float roughness = 0.5f;
        if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, roughness)) {
             resultMaterial->SetRoughness(1.0f - (roughness / 100.0f));
        }

        // 4. 加载纹理
        LoadMaterialTextures(material, resultMaterial, directory, device, cache);
        
        // 5. 上传 GPU
        resultMaterial->UpdateToGPU();

        return resultMaterial;
    }
    
    void ModelLoader::LoadMaterialTextures(aiMaterial* aiMat, 
                                         std::shared_ptr<Material> material, 
                                         const std::string& directory,
                                         RHI_Device* device,
                                         TextureCache& cache) {
        struct TexMapping {
            aiTextureType aiType;
            MaterialTextureSlot slot;
            bool sRGB;
        };

        std::vector<TexMapping> mappings = {
            { aiTextureType_DIFFUSE, MaterialTextureSlot::Albedo, true },
            { aiTextureType_BASE_COLOR, MaterialTextureSlot::Albedo, true }, 
            { aiTextureType_NORMALS, MaterialTextureSlot::Normal, false },
            { aiTextureType_METALNESS, MaterialTextureSlot::Metallic, false }, 
            { aiTextureType_DIFFUSE_ROUGHNESS, MaterialTextureSlot::Roughness, false },
            { aiTextureType_AMBIENT_OCCLUSION, MaterialTextureSlot::AO, false },
            { aiTextureType_EMISSIVE, MaterialTextureSlot::Emissive, true },
            { aiTextureType_UNKNOWN, MaterialTextureSlot::Metallic, false } 
        };

        for (const auto& map : mappings) {
            if (aiMat->GetTextureCount(map.aiType) > 0) {
                aiString str;
                aiMat->GetTexture(map.aiType, 0, &str);
                
                std::string filename = str.C_Str();
                // [关键] 路径拼接逻辑：先拼目录，如果不存在再试原名
                std::string fullPath = directory + "/" + filename;
                fullPath = NormalizePath(fullPath);

                if (!std::filesystem::exists(fullPath)) {
                    // 尝试在 textures 子目录下找 (常见 GLTF 结构)
                    std::string subPath = directory + "/textures/" + std::filesystem::path(filename).filename().string();
                    subPath = NormalizePath(subPath);
                    if (std::filesystem::exists(subPath)) {
                        fullPath = subPath;
                    } else {
                        // 尝试直接用文件名（假设在当前目录）
                        if (std::filesystem::exists(filename)) {
                            fullPath = filename;
                        }
                    }
                }

                // 缓存检查
                if (cache.loadedTextures.find(fullPath) != cache.loadedTextures.end()) {
                    TextureHandle handle = cache.loadedTextures[fullPath];
                    if (handle.IsValid()) {
                        material->SetTexture(map.slot, handle);
                    }
                    continue;
                }

                // 加载
                TextureHandle handle = LoadTextureRHI(fullPath, device, map.sRGB);
                if (handle.IsValid()) {
                    cache.loadedTextures[fullPath] = handle;
                    
                    if (map.aiType == aiTextureType_UNKNOWN) {
                         // GLTF ORM 贴图
                         material->SetTexture(MaterialTextureSlot::Metallic, handle);
                         material->SetTexture(MaterialTextureSlot::Roughness, handle);
                         material->SetTexture(MaterialTextureSlot::AO, handle);
                    } else {
                        material->SetTexture(map.slot, handle);
                    }
                    LOG_INFO("Loaded texture: " + fullPath);
                }
            }
        }
    }

    glm::vec3 ModelLoader::AssimpToGLM(aiVector3D vec) { return glm::vec3(vec.x, vec.y, vec.z); }
    glm::vec2 ModelLoader::AssimpToGLM(aiVector2D vec) { return glm::vec2(vec.x, vec.y); }
    glm::mat4 ModelLoader::AssimpToGLM(aiMatrix4x4 mat) {
        glm::mat4 result;
        result[0][0] = mat.a1; result[0][1] = mat.b1; result[0][2] = mat.c1; result[0][3] = mat.d1;
        result[1][0] = mat.a2; result[1][1] = mat.b2; result[1][2] = mat.c2; result[1][3] = mat.d2;
        result[2][0] = mat.a3; result[2][1] = mat.b3; result[2][2] = mat.c3; result[2][3] = mat.d3;
        result[3][0] = mat.a4; result[3][1] = mat.b4; result[3][2] = mat.c4; result[3][3] = mat.d4;
        return result;
    }

} // namespace HybridPBR