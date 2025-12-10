#pragma once
#include "../rhi/RHI_Device.h"
#include "../resources/Mesh.h"
#include <memory>

namespace HybridPBR {

    class GpuMesh {
    public:
        // 创建时就需要上传数据
        GpuMesh(RHI_Device* device, const std::shared_ptr<Mesh>& mesh);
        ~GpuMesh();

        // 销毁旧资源，上传新数据
        void Update(const std::shared_ptr<Mesh>& mesh);

        // RHI 访问接口
        BufferHandle GetVertexBuffer() const { return m_vertexBuffer; }
        BufferHandle GetIndexBuffer() const { return m_indexBuffer; }
        uint32_t GetIndexCount() const { return m_indexCount; }
        uint32_t GetVertexCount() const { return m_vertexCount; }
        bool HasIndices() const { return m_indexCount > 0; }

    private:
        RHI_Device* m_device; // 引用，不拥有
        
        BufferHandle m_vertexBuffer = BufferHandle::Invalid();
        BufferHandle m_indexBuffer = BufferHandle::Invalid();
        
        uint32_t m_vertexCount = 0;
        uint32_t m_indexCount = 0;
    };

} // namespace HybridPBR