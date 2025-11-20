#include "RenderPass.h"
#include "Rasterizer.h"
#include "utils/Logger.h"
#include <glm/gtx/string_cast.hpp>

namespace HybridPBR {

    // GeometryPass 实现
    void GeometryPass::Initialize() {
        LOG_INFO("Initializing GeometryPass");
    }

    void GeometryPass::Execute(const Scene& scene) {
        auto camera = scene.GetMainCamera();
        if (!camera) {
            LOG_WARNING("No main camera in scene for GeometryPass");
            return;
        }
        
        //SetBackfaceCulling(false);
        ApplyRenderState();
        
        // 渲染场景中的所有几何体
        RenderSceneNode(*scene.GetRoot(), glm::mat4(1.0f), scene);
    }

    void GeometryPass::Cleanup() {
        // 清理资源
    }

    void GeometryPass::ApplyRenderState() {
        // 线框模式
        if (wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        
        // 背面剔除
        if (backfaceCulling) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
        
        // 启用深度测试
        glEnable(GL_DEPTH_TEST);
    }

    void GeometryPass::RenderSceneNode(const SceneNode& node, const glm::mat4& parentTransform, const Scene& scene) {
        auto transform = parentTransform * node.GetTransform().GetLocalMatrix();

        
        // 渲染当前节点的网格
        if (auto mesh = node.GetMesh()) {
            if (auto material = node.GetMaterial()) {
                RenderMesh(*mesh, *material, transform);
            }
        }
        
        // 渲染子节点
        for (auto& child : node.GetChildren()) {
            RenderSceneNode(*child, transform, scene);
        }
    }

    void GeometryPass::RenderMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform) {
        // 获取材质对应的着色器
        auto shader = material.GetShader();
        if (!shader) {
            LOG_WARNING("Material has no valid shader, skipping render");
            return;
        }
        
        auto& shaderManager = ShaderManager::GetInstance();
        
        // 切换到材质对应的着色器
        shaderManager.SetCurrentShader(shader);
        
        // 设置公共统一变量
        SetupCommonUniforms(shader, *Rasterizer::GetCurrentScene(),material); 
        
        // 设置模型矩阵
        shader->SetMat4("model", transform);
        // 设置法线矩阵
        auto tmp = glm::transpose(glm::inverse(glm::mat3(transform)));
        shader->SetMat3("normalMatrix", tmp);
        
        // 应用材质特定参数
        material.ApplyToShader(shader);
        
        // 渲染网格
        mesh.Render();
        
        // 更新统计信息
        auto& stats = Rasterizer::GetStats();
        stats.drawCalls++;
        stats.triangleCount += mesh.GetTriangleCount();
        stats.vertexCount += mesh.GetVertexCount();
    }

    void GeometryPass::SetupCommonUniforms(std::shared_ptr<Shader> shader, const Scene& scene,const Material& material) {
        if (!shader) return;
        
        auto camera = scene.GetMainCamera();
        if (!camera) return;
        
        // 设置相机相关统一变量
        shader->SetMat4("view", camera->GetViewMatrix());
        shader->SetMat4("projection", camera->GetProjectionMatrix());
        shader->SetVec3("viewPos", camera->GetPosition());

        // 设置环境光
        shader->SetVec3("ambientLight", glm::vec3(0.05f)); // 添加默认环境光

        // 设置光照 (简化处理，只设置第一个方向光)
        int directionalLightCount = 0;
        for (const auto& light : scene.GetLights()) {
            if (!light->IsEnabled()) continue;
            
            if (light->GetType() == LightType::DIRECTIONAL && directionalLightCount == 0) {
                std::string prefix = "directionalLight";
                shader->SetVec3(prefix + ".direction", light->GetDirection());
                shader->SetVec3(prefix + ".color", light->GetProperties().color);
                shader->SetFloat(prefix + ".intensity", light->GetProperties().intensity);
                directionalLightCount++;
            }
        }
        //-----------------------------------------------------------------
        auto albedoMap = material.GetTexture(TextureType::DIFFUSE);
        auto normalMap = material.GetTexture(TextureType::NORMAL);
        auto metallicMap = material.GetTexture(TextureType::METALLIC);
        auto roughnessMap = material.GetTexture(TextureType::ROUGHNESS);
        auto aoMap = material.GetTexture(TextureType::AMBIENT_OCCLUSION);
        albedoMap ->Bind(0);
        normalMap ->Bind(1);
        metallicMap ->Bind(2);
        roughnessMap ->Bind(3);
        aoMap ->Bind(4);

        shader->SetInt("albedoMap", 0);
        shader->SetInt("normalMap", 1);
        shader->SetInt("metallicMap", 2);
        shader->SetInt("roughnessMap", 3);
        shader->SetInt("aoMap", 4);

        shader->SetVec3("lightPositions" , glm::vec3(0,0,10.0f));
        shader->SetVec3("lightColors" , glm::vec3(150.0f, 150.0f, 150.0f));
        shader->SetVec3("camPos" , camera->GetPosition());
        //------------------------------------------------------------------
        shader->SetInt("directionalLightCount", directionalLightCount);
        
        // 对于PBR着色器，设置IBL纹理（如果可用）
        // 只有在着色器确实存在时才进行比较，避免产生警告
        auto& shaderManager = ShaderManager::GetInstance();
        auto pbrShader = shaderManager.GetShader(ShaderType::PBR);
        // 检查获取到的着色器是否是默认着色器（意味着PBR着色器不存在）
        if (shader == pbrShader && pbrShader != shaderManager.GetDefaultShader()) {
            // 这里应该设置IBL纹理，暂时使用默认值
            shader->SetInt("irradianceMap", 6);
            shader->SetInt("prefilterMap", 7);
            shader->SetInt("brdfLUT", 8);
        }
    }

    // SkyboxPass 实现
    void SkyboxPass::Initialize() {
        LOG_INFO("Initializing SkyboxPass");
    }

    void SkyboxPass::Execute(const Scene& scene) {
        if (!skyboxTexture) return;
        
        auto camera = scene.GetMainCamera();
        if (!camera) return;
        
        auto& shaderManager = ShaderManager::GetInstance();
        auto skyboxShader = shaderManager.GetShader(ShaderType::SKYBOX);
        if (!skyboxShader) return;
        
        // 设置天空盒渲染状态
        glDepthFunc(GL_LEQUAL);  // 更改深度测试，让天空盒在远处
        glDisable(GL_CULL_FACE);
        
        shaderManager.SetCurrentShader(skyboxShader);
        
        // 设置天空盒统一变量
        skyboxShader->SetMat4("view", glm::mat4(glm::mat3(camera->GetViewMatrix()))); // 移除平移
        skyboxShader->SetMat4("projection", camera->GetProjectionMatrix());
        skyboxShader->SetInt("skybox", 0);
        skyboxTexture->Bind(0);
        
        // 渲染天空盒 (简化实现)
        // 实际应该渲染一个立方体
        
        // 恢复渲染状态
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
    }

    void SkyboxPass::Cleanup() {
        // 清理资源
    }

    // PostProcessPass 实现
    void PostProcessPass::Initialize() {
        LOG_INFO("Initializing PostProcessPass");
        
        // 加载后处理着色器
        auto& shaderManager = ShaderManager::GetInstance();
        postProcessShader = shaderManager.GetShader("PostProcess");
        if (!postProcessShader) {
            // 如果没有后处理着色器，创建一个简单的
            postProcessShader = std::make_shared<Shader>();
            // ... 创建简单后处理着色器
        }
        
        // 创建全屏四边形
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    void PostProcessPass::Execute(const Scene& scene) {
        if (!postProcessShader) return;
        
        auto& shaderManager = ShaderManager::GetInstance();
        shaderManager.SetCurrentShader(postProcessShader);
        
        // 绑定场景纹理到着色器
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        postProcessShader->SetInt("scene", 0);
        
        // 渲染全屏四边形
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    void PostProcessPass::Cleanup() {
        if (quadVAO) {
            glDeleteVertexArrays(1, &quadVAO);
            quadVAO = 0;
        }
        if (quadVBO) {
            glDeleteBuffers(1, &quadVBO);
            quadVBO = 0;
        }
    }

} // namespace HybridPBR