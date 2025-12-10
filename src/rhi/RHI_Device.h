#pragma once
#include "RHI_Types.h"
#include "RHI_CommandList.h"
#include <memory>
#include <vector>

namespace HybridPBR {

    struct BufferDesc {
        std::string name;
        uint64_t size;
        uint32_t usage; // BufferUsageBits 组合
        bool isDynamic = false; // 是否频繁更新 CPU -> GPU
    };

    struct TextureDesc {
        std::string name;
        uint32_t width;
        uint32_t height;
        TextureFormat format;
        bool isRenderTarget = false;
    };

    class RHI_Device {
    public:
        virtual ~RHI_Device() = default;

        // 初始化/销毁
        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;

        // 资源创建 (工厂模式)
        virtual BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) = 0;
        virtual TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr) = 0;
        
        // 创建着色器/管线 (简化版，先只传文件路径)
        virtual ShaderHandle CreateShader(const std::string& vertPath, const std::string& fragPath) = 0;
        // --- 管线状态 (Pipeline State) ---
        // 暂时我们只包含 Shader，后续会加入 BlendState, DepthStencilState, RasterizerState
        virtual PipelineHandle CreateSimplePipeline(ShaderHandle shader) = 0;
        virtual void DestroyPipeline(PipelineHandle handle) = 0;
        // 资源销毁
        virtual void DestroyBuffer(BufferHandle handle) = 0;
        virtual void DestroyTexture(TextureHandle handle) = 0;

        // 获取命令列表（用于记录绘制指令）
        virtual std::shared_ptr<RHI_CommandList> GetImmediateCommandList() = 0;

        virtual void UpdateBuffer(BufferHandle handle, const void* data, uint64_t size, uint64_t offset = 0) =0;
        // 帧管理
        virtual void BeginFrame() = 0;
        virtual void Present() = 0; // 交换缓冲区 (SwapBuffers)
    };

} // namespace HybridPBR