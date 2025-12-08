#include "Denoiser.h"
#include "utils/Logger.h"
#include "utils/GLCheck.h"
#include "utils/FileIO.h"
#include <glad/glad.h>

namespace HybridPBR {

    Denoiser::Denoiser() {
    }

    Denoiser::~Denoiser() {
        Shutdown();
    }

    bool Denoiser::Initialize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) {
            LOG_ERROR("Invalid dimensions for denoiser initialization");
            return false;
        }
        
        this->width = width;
        this->height = height;
        
        if (!CreateShaders()) {
            LOG_ERROR("Failed to create denoiser shaders");
            return false;
        }
        
        LOG_INFO("Denoiser initialized successfully");
        return true;
    }

    void Denoiser::Shutdown() {
        denoiserShader = nullptr;
        width = height = 0;
    }

    void Denoiser::Resize(uint32_t width, uint32_t height) {
        if (width == this->width && height == this->height) {
            return;
        }
        
        this->width = width;
        this->height = height;
    }

    void Denoiser::Denoise(const std::shared_ptr<Texture>& input, 
                          const std::shared_ptr<Texture>& output, 
                          float strength) {
        if (!denoiserShader || !input || !output) {
            return;
        }
        
        this->strength = strength;
        
        // 使用计算着色器进行降噪
        denoiserShader->Use();
        
        // 设置统一变量
        denoiserShader->SetInt("width", static_cast<int>(width));
        denoiserShader->SetInt("height", static_cast<int>(height));
        denoiserShader->SetFloat("strength", strength);
        
        // 绑定输入纹理
        input->Bind(0);
        denoiserShader->SetInt("inputTexture", 0);
        
        // 绑定输出纹理（作为图像）
        output->BindImage(0, 0,GL_WRITE_ONLY);
        
        // 分派计算着色器
        uint32_t groupsX = (width + 15) / 16;
        uint32_t groupsY = (height + 15) / 16;
        denoiserShader->Dispatch(groupsX, groupsY, 1);
        
        // 确保图像写入完成
        ComputeShader::ImageAccessBarrier();
        
        // 解绑
        glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        input->Unbind();
    }

    bool Denoiser::CreateShaders() {
        denoiserShader = std::make_shared<ComputeShader>();
        if (!denoiserShader->LoadFromFile(FileIO::GetAssetsPath() + "shaders/compute/denoiser.comp")) {
            LOG_ERROR("Failed to load denoiser shader");
            return false;
        }
        return true;
    }

} // namespace HybridPBR