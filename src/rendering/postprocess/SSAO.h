#pragma once
#include "rendering/common/Texture.h"
#include "rendering/rasterization/Camera.h"
#include "rendering/deferred/GBuffer.h"
#include "scene/Scene.h"
#include <memory>
#include <vector>
#include <random>

namespace HybridPBR {

    class SSAO {
    public:
        SSAO();
        ~SSAO();
        
        bool Initialize();
        void Shutdown();
        void Execute(const Scene& scene);
        void Resize(int width, int height);
        
        // 设置输入
        void SetGBuffer(std::shared_ptr<GBuffer> gbuffer) { this->gbuffer = gbuffer; }
        void SetCamera(std::shared_ptr<Camera> camera) { this->camera = camera; }
        
        // 参数设置
        void SetRadius(float radius) { this->radius = radius; }
        void SetBias(float bias) { this->bias = bias; }
        void SetPower(float power) { this->power = power; }
        
        // 获取输出
        std::shared_ptr<Texture> GetSSAOTexture() const { return ssaoTexture; }
        std::shared_ptr<Texture> GetBlurredTexture() const { return blurTexture; }

    private:
        std::shared_ptr<GBuffer> gbuffer;
        std::shared_ptr<Camera> camera;
        
        // SSAO纹理
        std::shared_ptr<Texture> ssaoTexture;
        std::shared_ptr<Texture> blurTexture;
        
        // 着色器
        std::shared_ptr<Shader> ssaoShader;
        std::shared_ptr<Shader> blurShader;
        
        // FBO
        uint32_t ssaoFBO = 0;
        uint32_t blurFBO = 0;
        
        // 采样核心
        std::vector<glm::vec3> ssaoKernel;
        std::shared_ptr<Texture> noiseTexture;
        
        // 参数
        float radius = 0.5f;
        float bias = 0.025f;
        float power = 2.0f;
        int kernelSize = 64;
        int noiseSize = 4;
        
        // 全屏四边形
        uint32_t quadVAO = 0, quadVBO = 0;
        
        // 初始化方法
        bool CreateSSAOFramebuffer(int width, int height);
        bool CreateBlurFramebuffer(int width, int height);
        void GenerateSamples();
        void GenerateNoiseTexture();
        void RenderFullscreenQuad();
        
        // 清理
        void Cleanup();
    };

} // namespace HybridPBR