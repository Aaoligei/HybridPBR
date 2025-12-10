#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <memory>

namespace HybridPBR {

    // 顶点定义保持不变
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoord;
        glm::vec3 tangent;
        glm::vec3 bitangent;
        
        Vertex() = default;
        Vertex(const glm::vec3& pos, const glm::vec3& norm = glm::vec3(0.0f), 
               const glm::vec2& tex = glm::vec2(0.0f))
            : position(pos), normal(norm), texcoord(tex) {}
    };

    /**
     * @brief CPU端网格数据容器
     * 只负责存储几何数据和算法生成，不涉及任何 GPU 操作。
     */
    class Mesh {
    public:
        Mesh(const std::string& name = "Mesh");
        ~Mesh() = default;
        
        // --- 数据设置 ---
        void SetVertices(const std::vector<Vertex>& vertices);
        void SetIndices(const std::vector<uint32_t>& indices);
        
        // --- 几何算法 (保留原有逻辑) ---
        void CalculateNormals();
        void CalculateTangents();
        void UpdateBounds();
        
        // --- 程序化生成 (保留原有逻辑) ---
        void GeneratePlane(float width, float height, uint32_t subdivisions = 1);
        void GenerateCube(float size = 1.0f);
        void GenerateSphere(float radius = 1.0f, uint32_t segments = 16);
        
        // --- Getters ---
        const std::string& GetName() const { return name; }
        const std::vector<Vertex>& GetVertices() const { return vertices; }
        const std::vector<uint32_t>& GetIndices() const { return indices; }
        
        // 统计信息
        uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
        uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }
        bool HasIndices() const { return !indices.empty(); }
        
        // 边界包围盒
        const glm::vec3& GetMinBounds() const { return minBounds; }
        const glm::vec3& GetMaxBounds() const { return maxBounds; }

    private:
        std::string name;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        
        glm::vec3 minBounds = glm::vec3(0.0f);
        glm::vec3 maxBounds = glm::vec3(0.0f);

        // 内部辅助
        void CalculateNormal(uint32_t i0, uint32_t i1, uint32_t i2);
    };

} // namespace HybridPBR