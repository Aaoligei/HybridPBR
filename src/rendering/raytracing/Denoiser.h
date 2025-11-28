#pragma once
#include "../common/Texture.h"
#include "ComputeShader.h"
#include <memory>

namespace HybridPBR {

    class Denoiser {
    public:
        Denoiser();
        ~Denoiser();
        
        bool Initialize(uint32_t width, uint32_t height);
        void Shutdown();
        void Resize(uint32_t width, uint32_t height);
        
        // 执行降噪处理
        void Denoise(const std::shared_ptr<Texture>& input, 
                    const std::shared_ptr<Texture>& output, 
                    float strength);
        
        // 设置降噪参数
        void SetStrength(float strength) { this->strength = strength; }
        void SetKernelSize(uint32_t size) { this->kernelSize = size; }

    private:
        bool CreateShaders();
        
        uint32_t width = 0;
        uint32_t height = 0;
        float strength = 0.5f;
        uint32_t kernelSize = 5;
        
        std::shared_ptr<ComputeShader> denoiserShader;
    };

} // namespace HybridPBR