#include "GpuMesh.h"
#include "utils/Logger.h"

namespace HybridPBR {

    GpuMesh::GpuMesh(RHI_Device* device, const std::shared_ptr<Mesh>& mesh)
        : m_device(device) {
        Update(mesh);
    }

    GpuMesh::~GpuMesh() {
        if (m_device) {
            m_device->DestroyBuffer(m_vertexBuffer);
            m_device->DestroyBuffer(m_indexBuffer);
        }
    }

    void GpuMesh::Update(const std::shared_ptr<Mesh>& mesh) {
        if (!mesh || !m_device) return;

        // 1. 清理旧资源
        if (m_vertexBuffer.IsValid()) m_device->DestroyBuffer(m_vertexBuffer);
        if (m_indexBuffer.IsValid()) m_device->DestroyBuffer(m_indexBuffer);

        const auto& vertices = mesh->GetVertices();
        const auto& indices = mesh->GetIndices();

        m_vertexCount = static_cast<uint32_t>(vertices.size());
        m_indexCount = static_cast<uint32_t>(indices.size());

        // 2. 创建顶点缓冲 (使用 RHI 接口)
        if (!vertices.empty()) {
            BufferDesc vboDesc;
            vboDesc.name = mesh->GetName() + "_VBO";
            vboDesc.size = vertices.size() * sizeof(Vertex);
            vboDesc.usage = (uint32_t)BufferUsageBits::VertexBuffer;
            
            m_vertexBuffer = m_device->CreateBuffer(vboDesc, vertices.data());
        }

        // 3. 创建索引缓冲 (使用 RHI 接口)
        if (!indices.empty()) {
            BufferDesc iboDesc;
            iboDesc.name = mesh->GetName() + "_IBO";
            iboDesc.size = indices.size() * sizeof(uint32_t);
            iboDesc.usage = (uint32_t)BufferUsageBits::IndexBuffer;
            
            m_indexBuffer = m_device->CreateBuffer(iboDesc, indices.data());
        }
        
        LOG_INFO("RHI", "Uploaded mesh to GPU: " + mesh->GetName());
    }

} // namespace HybridPBR