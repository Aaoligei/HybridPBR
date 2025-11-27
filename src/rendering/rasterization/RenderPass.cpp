#include "RenderPass.h"
#include "Rasterizer.h"
#include "utils/Logger.h"
#include "utils/GLCheck.h"
#include "GLFW/glfw3.h"
#include <glm/gtx/string_cast.hpp>

namespace HybridPBR {

    // GeometryPass 实现
    void GeometryPass::Initialize() {
        LOG_INFO("Initializing GeometryPass");
    }

    void GeometryPass::Execute(RenderContext& context) {
        auto& scene = context.scene;
        auto camera = scene->GetMainCamera();
        if (!camera) {
            LOG_WARNING("No main camera in scene for GeometryPass");
            return;
        }
        
        //SetBackfaceCulling(false);
        ApplyRenderState();
        
        // 渲染场景中的所有几何体
        RenderSceneNode(*scene->GetRoot(), glm::mat4(1.0f), *scene);
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

    void SkyboxPass::Execute(RenderContext& context) {
        auto& scene = context.scene;
        if (!skyboxTexture) return;
        
        auto camera = scene->GetMainCamera();
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
        
        // 简化的四边形渲染
        static unsigned int quadVAO = 0;
        static unsigned int quadVBO = 0;
        
        if (quadVAO == 0) {
            float quadVertices[] = {
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            };
            
            glCreateVertexArrays(1, &quadVAO);
            glCreateBuffers(1, &quadVBO);
            glNamedBufferStorage(quadVBO, sizeof(quadVertices), &quadVertices, 0);

            glVertexArrayVertexBuffer(quadVAO, 0, quadVBO, 0, 5 * sizeof(float));

            glEnableVertexArrayAttrib(quadVAO, 0);
            glVertexArrayAttribFormat(quadVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayAttribBinding(quadVAO, 0, 0);

            glEnableVertexArrayAttrib(quadVAO, 1);
            glVertexArrayAttribFormat(quadVAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
            glVertexArrayAttribBinding(quadVAO, 1, 0);

        }
    }

    void PostProcessPass::Execute(RenderContext& context) {
        if (!postProcessShader) return;
        
        auto& shaderManager = ShaderManager::GetInstance();
        shaderManager.SetCurrentShader(postProcessShader);
        
        // 绑定场景纹理到着色器
        glBindTextureUnit(GL_TEXTURE0, sceneTexture);
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

     // ================= GBufferPass =================
    void GBufferPass::Initialize() {
        // 获取专门的 GBuffer Shader
        auto& shaderManager = ShaderManager::GetInstance();
        shaderManager.LoadShader("GBuffer", 
                                FileIO::GetAssetsPath()+"shaders/deferred/gbuffer.vert",
                                FileIO::GetAssetsPath()+"shaders/deferred/gbuffer.frag");
        // 这个 Shader 输出必须对应 GBuffer 的 layout (pos, normal, albedo...)
        gBufferShader = ShaderManager::GetInstance().GetShader("GBuffer"); 
    }

    void GBufferPass::Execute(RenderContext& context) {
        if (!context.gBuffer || !gBufferShader) return;

        // 1. 绑定 GBuffer FBO 进行写入
        context.gBuffer->BindForGeometryPass(); // 内部调用 glBindFramebuffer + glDrawBuffers
        
        // 2. 设置状态
        glViewport(0, 0, context.width, context.height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        // 清除 GBuffer 的颜色和深度
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

        glEnable(GL_DEPTH_TEST);
        if (backfaceCulling) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // 3. 激活 Shader
        ShaderManager::GetInstance().SetCurrentShader(gBufferShader);

        // 4. 遍历场景渲染
        // 注意：这里我们不再使用 Material 自带的 Shader，而是强制使用 GBufferShader
        // 但我们仍然使用 Material 的纹理
        if (context.scene->GetRoot()) {
            RenderSceneNode(*context.scene->GetRoot(), glm::mat4(1.0f), *context.scene);
        }

        // 5. 解绑
        //context.gBuffer->Unbind();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // 恢复填充模式
    }

    void GBufferPass::RenderSceneNode(const SceneNode& node, const glm::mat4& parentTransform,const Scene& scene) {
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

    void GBufferPass::RenderMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform) {

        auto& shaderManager = ShaderManager::GetInstance();
        shaderManager.SetCurrentShader(gBufferShader);
        
        // 1. 设置模型矩阵 (Per Object)
        gBufferShader->SetMat4("model", transform);
        
        // 计算法线矩阵 (在Shader里计算开销较大，骨骼动画除外)
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
        gBufferShader->SetMat3("normalMatrix", normalMatrix);
        
        // 2. 应用材质 (Per Material)
        // 这里调用 Material 自己的 ApplyToShader，不要在 Pass 里手动绑定纹理
        material.ApplyToShader(gBufferShader);
        
        mesh.Render();
        
        // 更新统计信息
        auto& stats = Rasterizer::GetStats();
        stats.drawCalls++;
        stats.triangleCount += mesh.GetTriangleCount();
        stats.vertexCount += mesh.GetVertexCount();
    }
    void GBufferPass::Cleanup() {
        // 清理资源
    }

     // ================= LightingPass =================
    void LightingPass::Initialize() {
        auto& shaderManager = ShaderManager::GetInstance();
        shaderManager.LoadShader("DeferredLighting", 
                                FileIO::GetAssetsPath()+"shaders/deferred/lighting.vert",
                                FileIO::GetAssetsPath()+"shaders/deferred/lighting.frag");
        lightingShader = ShaderManager::GetInstance().GetShader("DeferredLighting");

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

    void LightingPass::Execute(RenderContext& context) {
        if (!context.gBuffer || !lightingShader) return;

        // 1. 绑定输出 FBO (通常是默认 FBO 或者 HDR 纹理 FBO)
        glBindFramebuffer(GL_FRAMEBUFFER, context.outputFBO);
        glClear(GL_COLOR_BUFFER_BIT); // 只需要清颜色，不需要清深度(因为要留给 Skybox 用)
        
        glDisable(GL_DEPTH_TEST); // 光照计算是全屏 Quad，不需要深度测试
        glDisable(GL_CULL_FACE);

        ShaderManager::GetInstance().SetCurrentShader(lightingShader);

        // 2. 绑定 GBuffer 纹理资源到 Shader
        // 假设 TextureUnit 0-4 分配给 GBuffer
        context.gBuffer->BindForLightingPass();
        
        // 设置 Shader 中的采样器索引
        lightingShader->SetInt("gPosition", 0);
        lightingShader->SetInt("gNormal", 1);
        lightingShader->SetInt("gAlbedo", 2);
        // ...

        // 3. 更新光照 Uniform (如果 Rasterizer 的 UBO 不够用，可以在这里传额外的)
        
        // 4. 绘制全屏四边形
        RenderQuad();
        
        // 5. 关键步骤：Blit Depth Buffer
        // 将 GBuffer 的深度缓冲复制到当前的 Output FBO
        // 这样后续的 SkyboxPass 和 ForwardTransparentPass 才能正确进行深度测试
        glBindFramebuffer(GL_READ_FRAMEBUFFER, context.gBuffer->GetFBO());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, context.outputFBO); 
        glBlitFramebuffer(0, 0, context.width, context.height, 
                          0, 0, context.width, context.height, 
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        
        glEnable(GL_DEPTH_TEST); // 恢复深度测试供后续 Pass 使用
    }
    void LightingPass::Cleanup() {
        // 清理资源
    }

    void LightingPass::RenderQuad() {
        // 简化的四边形渲染
        static unsigned int quadVAO = 0;
        static unsigned int quadVBO = 0;
        
        if (quadVAO == 0) {
            float quadVertices[] = {
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            };
            
            glCreateVertexArrays(1, &quadVAO);
            glCreateBuffers(1, &quadVBO);
            glNamedBufferStorage(quadVBO, sizeof(quadVertices), &quadVertices, 0);

            glVertexArrayVertexBuffer(quadVAO, 0, quadVBO, 0, 5 * sizeof(float));

            glEnableVertexArrayAttrib(quadVAO, 0);
            glVertexArrayAttribFormat(quadVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayAttribBinding(quadVAO, 0, 0);

            glEnableVertexArrayAttrib(quadVAO, 1);
            glVertexArrayAttribFormat(quadVAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
            glVertexArrayAttribBinding(quadVAO, 1, 0);

        }
        
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }


} // namespace HybridPBR