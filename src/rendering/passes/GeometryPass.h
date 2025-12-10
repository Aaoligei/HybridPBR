#pragma once
#include "RenderPass.h"
#include "../GpuMesh.h"
#include <unordered_map>

namespace HybridPBR {

    class GeometryPass : public RenderPass {
    public:
        GeometryPass() = default;
        ~GeometryPass() override = default;

        void Initialize(RHI_Device* device) override;
        void Execute(const RenderContext& context) override;
        void Cleanup() override;

        std::string GetName() const override { return "GeometryPass"; }

    private:
        // 简单的 GPU 资源缓存
        // Key: Mesh ID (或指针), Value: GpuMesh
        // 注意：实际项目中应该由 ResourceManager 管理
        std::unordered_map<const void*, std::unique_ptr<GpuMesh>> m_gpuMeshCache;
        
        // 新增：全局 UBO
        BufferHandle m_globalUBO = BufferHandle::Invalid();
        BufferHandle m_lightUBO = BufferHandle::Invalid();
        // 默认管线 (Shader)
        PipelineHandle m_defaultPipeline = PipelineHandle::Invalid();
        ShaderHandle m_defaultShader = ShaderHandle::Invalid();
        
        RHI_Device* m_device = nullptr; // 缓存 Device 指针

        // 辅助：获取或创建 GpuMesh
        GpuMesh* GetOrCreateGpuMesh(RHI_Device* device, const std::shared_ptr<Mesh>& mesh);
        void UpdateGlobalState(const RenderContext& context);
    };

} // namespace HybridPBR