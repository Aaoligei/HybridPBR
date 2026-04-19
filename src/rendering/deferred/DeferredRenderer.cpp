#include "DeferredRenderer.h"

namespace HybridPBR {
    RenderStats DeferredRenderer::stats;
    // DeferredRenderer 实现
    DeferredRenderer::DeferredRenderer() {
        gBufferPass = std::make_unique<GBufferPass>();
        lightingPass = std::make_unique<LightingPass>();
        skyboxPass = std::make_unique<SkyboxPass>();
        ssaoPass = std::make_unique<SSAO>();
        stats = RenderStats();
    }

    DeferredRenderer::~DeferredRenderer() {
        Shutdown();
    }

    bool DeferredRenderer::Initialize(int width, int height) {
        LOG_INFO("Initializing DeferredRenderer");
        
        // 初始化几何通道
        gBufferPass->Initialize();
        gBufferPass->Resize(width, height);
        
        // 初始化光照通道
        lightingPass->Initialize();
        lightingPass->SetGBuffer(gBufferPass->GetGBuffer());

        // 初始化skybox通道
        skyboxPass->Initialize();
        
        // 创建UBO（记得延迟渲染是2和3）
        cameraUBO = std::make_unique<UniformBuffer>(sizeof(CameraData), 2);
        lightUBO = std::make_unique<UniformBuffer>(sizeof(LightData), 3);

        // 初始化SSAO
        if (ssaoEnabled) {
            ssaoPass->Initialize();
            ssaoPass->Resize(width, height);
        }
        
        // 创建输出FBO
        if (!CreateOutputFramebuffer(width, height)) {
            return false;
        }
        
        initialized = true;
        LOG_INFO("DeferredRenderer initialized successfully");
        
        return true;
    }

    void DeferredRenderer::Shutdown() {
        if (outputFBO) {
            glDeleteFramebuffers(1, &outputFBO);
            outputFBO = 0;
        }
        
        initialized = false;
        LOG_INFO("DeferredRenderer shutdown");
    }

    void DeferredRenderer::Render(const Scene& scene) {
        stats.Reset();
        if (!initialized) return;
        
        auto camera = scene.GetMainCamera();
        if (!camera) {
            LOG_WARNING("No main camera in scene");
            return;
        }
        
        UpdateGlobalUniforms(scene);

        // Execute shadow pass
        if (m_shadowPass) {
            m_shadowPass->Execute(scene);
        }
        
        // 绑定输出FBO
        glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
        glViewport(0, 0, outputTexture->GetWidth(), outputTexture->GetHeight());
        glClearColor(0.0f, 1.0f, 0.0f, 1.0f); // 纯绿色
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 执行几何通道
        gBufferPass->Execute(scene);
        
        // 执行SSAO（如果启用）
        if (ssaoEnabled) {
            ssaoPass->SetGBuffer(gBufferPass->GetGBuffer());
            ssaoPass->SetCamera(camera);
            ssaoPass->Execute(scene);
        }
        
        // 绑定输出FBO进行光照计算
        glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
        
        // 执行光照通道
        if (m_shadowPass) {
            //lightingPass->SetShadowMap(m_shadowPass->GetShadowMapTexture());
        }
        lightingPass->Execute(scene);

        // [新增] === 天空盒渲染逻辑 ===
        if (skyboxPass) {
            // 1. 拷贝深度信息：从 G-Buffer FBO 到 Output FBO
            // 这样天空盒才能通过深度测试（只在没有物体的地方绘制）
            glBindFramebuffer(GL_READ_FRAMEBUFFER, gBufferPass->GetGBuffer()->GetFBO());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outputFBO); 
            
            glBlitFramebuffer(
                0, 0, gBufferPass->GetGBuffer()->GetWidth(), gBufferPass->GetGBuffer()->GetHeight(), // Src Rect
                0, 0, outputTexture->GetWidth(), outputTexture->GetHeight(),                         // Dst Rect
                GL_DEPTH_BUFFER_BIT, // 拷贝深度
                GL_NEAREST           // 过滤模式
            );
            
            // 2. 重新绑定 Output FBO 进行绘制
            glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
            
            // 3. 渲染天空盒 (SkyboxPass 内部会设置 DepthFunc 为 LEQUAL)
            skyboxPass->Execute(scene);
        }
        
        // 解除FBO绑定
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // [新增代码开始] ------------------------------------------------
        glBindFramebuffer(GL_READ_FRAMEBUFFER, outputFBO); // 源：你的渲染结果
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);         // 目标：屏幕
        
        // 这里的 width/height 应该是窗口的大小
        // 假设 outputTexture 大小和窗口一致
        glBlitFramebuffer(
            0, 0, outputTexture->GetWidth(), outputTexture->GetHeight(), // src rect
            0, 0, outputTexture->GetWidth(), outputTexture->GetHeight(), // dst rect
            GL_COLOR_BUFFER_BIT, // 只需要拷贝颜色
            GL_NEAREST           // 1:1 拷贝用 NEAREST 即可
        );
        // [新增代码结束] ------------------------------------------------
    }

    void DeferredRenderer::Resize(int width, int height) {
        if (!initialized) return;
        
        gBufferPass->Resize(width, height);
        
        if (ssaoEnabled) {
            ssaoPass->Resize(width, height);
        }
        
        CreateOutputFramebuffer(width, height);
    }

    void DeferredRenderer::SetIBLSystem(std::shared_ptr<IBL> ibl) {
        lightingPass->SetIBLSystem(ibl);
        skyboxPass->SetSkyboxTexture(ibl->GetEnvironmentMap());
    }

    void DeferredRenderer::SetWireframe(bool enabled) {
        gBufferPass->SetWireframe(enabled);
    }

    void DeferredRenderer::SetShadowPass(std::shared_ptr<ShadowPass> shadowPass)
    {
        m_shadowPass = shadowPass;
    }

    std::shared_ptr<Texture> DeferredRenderer::GetOutputTexture() const {
        return outputTexture;
    }

    bool DeferredRenderer::CreateOutputFramebuffer(int width, int height) {
        if (outputFBO) {
            glDeleteFramebuffers(1, &outputFBO);
        }
        
        glCreateFramebuffers(1, &outputFBO);
        
        // 创建输出纹理
        outputTexture = std::make_shared<Texture>();
        if (!outputTexture->Create2D(width, height, GL_RGBA16F, GL_RGBA, GL_FLOAT)) {
            LOG_ERROR("Failed to create deferred output texture");
            return false;
        }
        
        outputTexture->SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        outputTexture->SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        
        glNamedFramebufferTexture(outputFBO, GL_COLOR_ATTACHMENT0, outputTexture->GetID(), 0);
        
        // 创建深度渲染缓冲区
        uint32_t depthRBO;
        glCreateRenderbuffers(1, &depthRBO);
        glNamedRenderbufferStorage(depthRBO, GL_DEPTH_COMPONENT32F, width, height);
        glNamedFramebufferRenderbuffer(outputFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
        
        if (glCheckNamedFramebufferStatus(outputFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Deferred output framebuffer is not complete!");
            return false;
        }
        
        return true;
    }

     void DeferredRenderer::UpdateGlobalUniforms(const Scene& scene) {
        auto camera = scene.GetMainCamera();
        if (camera) {
            CameraData camData;
            camData.view = camera->GetViewMatrix();
            camData.projection = camera->GetProjectionMatrix();
            camData.viewPos = camera->GetPosition();
            cameraUBO->SetData(&camData, sizeof(CameraData));
        }

        // 收集光源数据 (only enabled lights)
        LightData lightData;
        const auto& lights = scene.GetLights();
        lightData.lightCount = 0;
        
        for(int i=0; i < std::min((int)lights.size(), 16); ++i) {
            auto& l = lights[i];
            if (!l->IsEnabled()) continue;
            
            auto& props = l->GetProperties();
            int idx = lightData.lightCount;
            
            lightData.lights[idx].position = l->GetPosition();
            lightData.lights[idx].direction = l->GetDirection();
            lightData.lights[idx].color = props.color;
            lightData.lights[idx].intensity = props.intensity;

            lightData.lights[idx].range = props.range;
            lightData.lights[idx].constant = props.constant;
            lightData.lights[idx].linear = props.linear;
            lightData.lights[idx].quadratic = props.quadratic;

            lightData.lights[idx].innerCutoff = props.innerCutoff;
            lightData.lights[idx].outerCutoff = props.outerCutoff;
            lightData.lights[idx].type = (int)l->GetType();
            lightData.lightCount++;
        }
        
        lightUBO->SetData(&lightData, sizeof(LightData));
    }
}