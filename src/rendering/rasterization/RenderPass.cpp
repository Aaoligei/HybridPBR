#include "RenderPass.h"
#include "Rasterizer.h"
#include "utils/Logger.h"
#include"utils/GLCall.h"
#include "GLFW/glfw3.h"
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
    // 检查是否有有效的OpenGL上下文
    if (!glfwGetCurrentContext()) {
        LOG_ERROR("No valid OpenGL context!");
        return;
    }
    
    // 线框模式
     if (wireframe) {
         glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
     } else {
         glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
     }
    
    // 背面剔除
    if (backfaceCulling) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }
    
    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // 确保深度写入开启
    glDepthMask(GL_TRUE);
    
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
        shaderManager.SetCurrentShader(shader);
        
        // 1. 设置模型矩阵 (Per Object)
        shader->SetMat4("model", transform);
        
        // 计算法线矩阵 (在Shader里计算开销较大，骨骼动画除外)
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
        shader->SetMat3("normalMatrix", normalMatrix);
        
        // 2. 应用材质 (Per Material)
        // 这里调用 Material 自己的 ApplyToShader，不要在 Pass 里手动绑定纹理
        material.ApplyToShader(shader);
        
        mesh.Render();
        
        // 更新统计信息
        auto& stats = Rasterizer::GetStats();
        stats.drawCalls++;
        stats.triangleCount += mesh.GetTriangleCount();
        stats.vertexCount += mesh.GetVertexCount();
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
        
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        // 1. 保存旧状态
        GLint oldDepthFunc, oldCullFace;
        GLboolean oldDepthMask;
        glGetIntegerv(GL_DEPTH_FUNC,      &oldDepthFunc);
        glGetIntegerv(GL_CULL_FACE_MODE,  &oldCullFace);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);

        //设天空盒专用状态
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);   // 不写深度
        
        shaderManager.SetCurrentShader(skyboxShader);
        
        // 设置天空盒统一变量
        skyboxShader->SetMat4("view", glm::mat4(glm::mat3(camera->GetViewMatrix()))); // 移除平移
        skyboxShader->SetMat4("projection", camera->GetProjectionMatrix());
        skyboxShader->SetInt("environmentMap", 0);
        skyboxTexture->Bind(0);
        
        if (cubeVAO == 0){
            glGenVertexArrays(1, &cubeVAO);
            glGenBuffers(1, &cubeVBO);
            // fill buffer
            glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
            // link vertex attributes
            glBindVertexArray(cubeVAO);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
        // render Cube
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        
        // 恢复渲染状态
        glDepthFunc(oldDepthFunc);
        glDepthMask(oldDepthMask);
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