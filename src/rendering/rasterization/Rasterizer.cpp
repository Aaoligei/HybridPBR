#include "Rasterizer.h"

namespace HybridPBR {

    Rasterizer::Rasterizer() {
        stats = RenderStats();
    }

    Rasterizer::~Rasterizer() {
        Shutdown();
    }

    bool Rasterizer::Initialize() {
        LOG_INFO("Initializing Rasterizer");
        
        // 设置OpenGL状态
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        
        // 设置默认混合
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // 创建默认着色器
        if (!SetupDefaultShaders()) {
            LOG_ERROR("Failed to setup default shaders");
            return false;
        }
        
        LOG_INFO("Rasterizer initialized successfully");
        return true;
    }

    void Rasterizer::Shutdown() {
        if (defaultShader) {
            defaultShader->Destroy();
        }
        if (skyboxShader) {
            skyboxShader->Destroy();
        }
        
        LOG_INFO("Rasterizer shutdown");
    }

    void Rasterizer::Render(const Scene& scene) {
        BeginFrame();
        
        // 获取主相机
        auto camera = scene.GetMainCamera();
        if (!camera) {
            LOG_WARNING("No main camera set in scene");
            return;
        }
        
        // 设置视图和投影
        auto viewMatrix = camera->GetViewMatrix();
        auto projectionMatrix = camera->GetProjectionMatrix();
        
        // 更新着色器相机统一变量
        defaultShader->Use();
        defaultShader->SetMat4("view", viewMatrix);
        defaultShader->SetMat4("projection", projectionMatrix);
        //defaultShader->SetVec3("viewPos", camera->GetPosition());
        
        // 设置光照
        SetupLighting(defaultShader, scene);
        
        // 渲染场景节点
        RenderSceneNode(*scene.GetRoot(), glm::mat4(1.0f));
        
        // 渲染天空盒
        if (skyboxTexture) {
            RenderSkybox();
        }
        
        EndFrame();
    }

    void Rasterizer::BeginFrame() {
        stats.Reset();
        
        // 清除缓冲区
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        ApplyRenderState();
    }

    void Rasterizer::EndFrame() {
        // 更新帧时间统计
        // 注意：实际帧时间应该在应用程序级别测量
        
        // 在这里我们可以做一些帧结束时的清理工作或者统计信息的更新
        // 当前帧的绘制调用次数、三角形数量和顶点数量已经在渲染过程中更新
        
        // 可以在这里添加一些OpenGL的状态重置或者其他清理工作
        // 例如：检查OpenGL错误
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            LOG_WARNING("OpenGL error in EndFrame: " + std::to_string(error));
        }
    }

    void Rasterizer::SetViewport(int width, int height) {
        glViewport(0, 0, width, height);
    }

    void Rasterizer::SetClearColor(const glm::vec4& color) {
        clearColor = color;
    }

    void Rasterizer::SetWireframe(bool enabled) {
        wireframe = enabled;
        ApplyRenderState();
    }

    void Rasterizer::SetBackfaceCulling(bool enabled) {
        backfaceCulling = enabled;
        ApplyRenderState();
    }

    void Rasterizer::SetDepthTest(bool enabled) {
        depthTest = enabled;
        ApplyRenderState();
    }

    void Rasterizer::SetSkybox(std::shared_ptr<Texture> skybox) {
        skyboxTexture = skybox;
    }

    void Rasterizer::SetAmbientLight(const glm::vec3& color, float intensity) {
        ambientLight = color * intensity;
    }

    void Rasterizer::RenderSceneNode(const SceneNode& node, const glm::mat4& parentTransform) {
        auto transform = parentTransform * node.GetTransform().GetLocalMatrix();
        
        // 渲染当前节点的网格
        if (auto mesh = node.GetMesh()) {
            if (auto material = node.GetMaterial()) {
                RenderMesh(*mesh, *material, transform);
            }
        }
        
        // 渲染子节点
        for (auto& child : node.GetChildren()) {
            RenderSceneNode(*child, transform);
        }
    }

    void Rasterizer::RenderMesh(const Mesh& mesh, const Material& material, const glm::mat4& transform) {
        if (!defaultShader) return;
        
        // 设置模型矩阵
        defaultShader->SetMat4("model", transform);
        
        // 应用材质
        material.ApplyToShader(defaultShader);
        
        // 渲染网格
        mesh.Render();
        
        // 更新统计
        stats.drawCalls++;
        stats.triangleCount += mesh.GetTriangleCount();
        stats.vertexCount += mesh.GetVertexCount();
    }

    void Rasterizer::RenderSkybox() {
        if (!skyboxShader || !skyboxTexture) return;
        
        // 暂时禁用深度写入，天空盒应该在最后渲染
        glDepthMask(GL_FALSE);
        
        skyboxShader->Use();
        skyboxTexture->Bind(0);
        skyboxShader->SetInt("skybox", 0);
        
        // 渲染一个立方体（简化处理，实际应该使用专门的天空盒网格）
        // 这里简化实现
        
        glDepthMask(GL_TRUE);
    }

    void Rasterizer::SetupLighting(std::shared_ptr<Shader> shader, const Scene& scene) {
        if (!shader) return;
        
        shader->Use();
        
        // 设置环境光
        shader->SetVec3("ambientLight", ambientLight);
        
        // 设置方向光（简化处理，只使用第一个方向光）
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
            
            // 可以添加点光源和聚光灯的支持
        }
        
        shader->SetInt("directionalLightCount", directionalLightCount);
    }

    void Rasterizer::ApplyRenderState() {
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
        
        // 深度测试
        if (depthTest) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }

    bool Rasterizer::SetupDefaultShaders() {
        // 创建默认着色器
        defaultShader = ResourceManager::GetInstance().LoadShader("default", 
            FileIO::GetAssetsPath()+"shaders/default.vert", FileIO::GetAssetsPath()+"shaders/default.frag");
        
        // 创建天空盒着色器
        skyboxShader = ResourceManager::GetInstance().LoadShader("skybox",
            FileIO::GetAssetsPath()+"shaders/skybox.vert", FileIO::GetAssetsPath()+"shaders/skybox.frag");
        
        return defaultShader != nullptr && skyboxShader != nullptr;
    }

} // namespace HybridPBR