#pragma once
#include "../common/Texture.h"
#include <memory>
#include <vector>

namespace HybridPBR {

    enum class GBufferTextureType {
        Position = 0,
        Normal,
        Albedo,
        MetallicRoughnessAO, // 金属度、粗糙度、AO打包
        Emissive,
        Depth,
        Count
    };

    class GBuffer {
    public:
        GBuffer();
        ~GBuffer();
        
        bool Initialize(int width, int height);
        void Destroy();
        void Resize(int width, int height);
        
        // 绑定G-Buffer进行几何通道渲染
        void BindForGeometryPass();
        
        // 绑定G-Buffer进行光照通道读取
        void BindForLightingPass();
        
        // 绑定特定纹理用于读取
        void BindTexture(GBufferTextureType type, uint32_t unit) const;
        
        // 获取纹理
        std::shared_ptr<Texture> GetTexture(GBufferTextureType type) const;
        
        // 状态查询
        bool IsValid() const { return initialized; }
        int GetWidth() const { return width; }
        int GetHeight() const { return height; }
        uint32_t GetFBO() const { return fbo; }
        
        // 调试可视化
        void BindForDebugVisualization(GBufferTextureType type, uint32_t unit) const;

    private:
        int width = 0;
        int height = 0;
        bool initialized = false;
        
        uint32_t fbo = 0;
        uint32_t depthRBO = 0;
        
        std::vector<std::shared_ptr<Texture>> textures;
        
        // 初始化纹理
        bool CreateTextures();
        bool CreateFramebuffer();
        
        // 设置纹理参数
        void SetupTextureParameters();
    };

} // namespace HybridPBR