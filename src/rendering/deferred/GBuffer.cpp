#include "GBuffer.h"
#include "utils/Logger.h"

namespace HybridPBR {

    GBuffer::GBuffer() {
        textures.resize(static_cast<size_t>(GBufferTextureType::Count));
    }

    GBuffer::~GBuffer() {
        Destroy();
    }

    bool GBuffer::Initialize(int w, int h) {
        if (initialized) {
            Destroy();
        }
        
        width = w;
        height = h;
        
        if (!CreateTextures() || !CreateFramebuffer()) {
            LOG_ERROR("Failed to initialize GBuffer");
            return false;
        }
        
        initialized = true;
        LOG_INFO("GBuffer initialized: " + std::to_string(width) + "x" + std::to_string(height));
        return true;
    }

    void GBuffer::Destroy() {
        if (fbo) {
            glDeleteFramebuffers(1, &fbo);
            fbo = 0;
        }
        
        if (depthRBO) {
            glDeleteRenderbuffers(1, &depthRBO);
            depthRBO = 0;
        }
        
        for (auto& texture : textures) {
            if (texture) {
                // 纹理会被智能指针自动管理
            }
        }
        textures.clear();
        
        initialized = false;
    }

    void GBuffer::Resize(int w, int h) {
        if (w == width && h == height) return;
        
        Initialize(w, h);
    }

    void GBuffer::BindForGeometryPass() {
        if (!initialized) return;
        
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, width, height);
        
        // 清除所有附件
        GLenum drawBuffers[] = {
            GL_COLOR_ATTACHMENT0, // Position
            GL_COLOR_ATTACHMENT1, // Normal
            GL_COLOR_ATTACHMENT2, // Albedo
            GL_COLOR_ATTACHMENT3, // MetallicRoughnessAO
            GL_COLOR_ATTACHMENT4  // Emissive
        };
        glDrawBuffers(5, drawBuffers);
        
        // 清除颜色和深度
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GBuffer::BindForLightingPass() {
        if (!initialized) return;
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // 绑定到默认FBO
        
        // 绑定所有G-Buffer纹理用于读取
        for (int i = 0; i < static_cast<int>(GBufferTextureType::Count) - 1; ++i) { // 排除深度
            BindTexture(static_cast<GBufferTextureType>(i), i);
        }
    }

    void GBuffer::BindTexture(GBufferTextureType type, uint32_t unit) const {
        if (!initialized || type == GBufferTextureType::Count) return;
        
        auto texture = textures[static_cast<size_t>(type)];
        if (texture) {
            texture->Bind(unit);
        }
    }

    std::shared_ptr<Texture> GBuffer::GetTexture(GBufferTextureType type) const {
        if (!initialized || type == GBufferTextureType::Count) {
            return nullptr;
        }
        return textures[static_cast<size_t>(type)];
    }

    void GBuffer::BindForDebugVisualization(GBufferTextureType type, uint32_t unit) const {
        BindTexture(type, unit);
    }

    bool GBuffer::CreateTextures() {
        // 位置 (RGB16F - 高精度世界空间位置)
        auto positionTex = std::make_shared<Texture>();
        if (!positionTex->Create2D(width, height, GL_RGB16F, GL_RGB, GL_FLOAT)) {
            return false;
        }
        textures[static_cast<size_t>(GBufferTextureType::Position)] = positionTex;
        
        // 法线 (RGB16F - 世界空间法线)
        auto normalTex = std::make_shared<Texture>();
        if (!normalTex->Create2D(width, height, GL_RGB16F, GL_RGB, GL_FLOAT)) {
            return false;
        }
        textures[static_cast<size_t>(GBufferTextureType::Normal)] = normalTex;
        
        // 反照率 (RGBA8 - sRGB颜色)
        auto albedoTex = std::make_shared<Texture>();
        if (!albedoTex->Create2D(width, height, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE)) {
            return false;
        }
        textures[static_cast<size_t>(GBufferTextureType::Albedo)] = albedoTex;
        
        // 金属度、粗糙度、AO (RGBA8 - 分别存储在RGB通道)
        auto mraTex = std::make_shared<Texture>();
        if (!mraTex->Create2D(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE)) {
            return false;
        }
        textures[static_cast<size_t>(GBufferTextureType::MetallicRoughnessAO)] = mraTex;
        
        // 自发光 (RGB16F - HDR发射颜色)
        auto emissiveTex = std::make_shared<Texture>();
        if (!emissiveTex->Create2D(width, height, GL_RGB16F, GL_RGB, GL_FLOAT)) {
            return false;
        }
        textures[static_cast<size_t>(GBufferTextureType::Emissive)] = emissiveTex;
        
        SetupTextureParameters();
        return true;
    }

    bool GBuffer::CreateFramebuffer() {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        
        // 附加颜色附件
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
                              textures[static_cast<size_t>(GBufferTextureType::Position)]->GetID(), 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 
                              textures[static_cast<size_t>(GBufferTextureType::Normal)]->GetID(), 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, 
                              textures[static_cast<size_t>(GBufferTextureType::Albedo)]->GetID(), 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, 
                              textures[static_cast<size_t>(GBufferTextureType::MetallicRoughnessAO)]->GetID(), 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, 
                              textures[static_cast<size_t>(GBufferTextureType::Emissive)]->GetID(), 0);
        
        // 创建深度渲染缓冲区
        glGenRenderbuffers(1, &depthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
        
        // 检查完整性
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Framebuffer is not complete!");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    void GBuffer::SetupTextureParameters() {
        for (auto& texture : textures) {
            if (texture) {
                texture->SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
                texture->SetFilter(TextureFilter::NEAREST, TextureFilter::NEAREST); // 延迟渲染通常使用NEAREST
            }
        }
    }

} // namespace HybridPBR