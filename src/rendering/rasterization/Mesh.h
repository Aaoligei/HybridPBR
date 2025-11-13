#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../common/Material.h"

namespace HybridPBR {

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

    class Mesh {
    public:
        Mesh(const std::string& name = "Mesh");
        ~Mesh();
        
        // 几何数据设置
        void SetVertices(const std::vector<Vertex>& vertices);
        void SetIndices(const std::vector<uint32_t>& indices);
        
        // 网格构建
        void CalculateNormals();
        void CalculateTangents();
        void GeneratePlane(float width, float height, uint32_t subdivisions = 1);
        void GenerateCube(float size = 1.0f);
        void GenerateSphere(float radius = 1.0f, uint32_t segments = 16);
        
        // 渲染
        void Render() const;
        void RenderInstanced(uint32_t instanceCount) const;
        
        // 状态查询
        bool IsValid() const;
        bool HasIndices() const { return !indices.empty(); }
        
        // 获取信息
        const std::string& GetName() const { return name; }
        uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
        uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }
        uint32_t GetTriangleCount() const { return GetIndexCount() / 3; }
        
        // 边界体积
        const glm::vec3& GetMinBounds() const { return minBounds; }
        const glm::vec3& GetMaxBounds() const { return maxBounds; }
        glm::vec3 GetCenter() const { return (minBounds + maxBounds) * 0.5f; }
        float GetBoundingRadius() const;

    private:
        std::string name;
        
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        
        // OpenGL对象
        uint32_t VAO = 0;
        uint32_t VBO = 0;
        uint32_t EBO = 0;
        
        // 边界框
        glm::vec3 minBounds = glm::vec3(FLT_MAX);
        glm::vec3 maxBounds = glm::vec3(-FLT_MAX);
        
        bool buffersInitialized = false;
        
        void SetupBuffers();
        void UpdateBounds();
        void CalculateNormal(uint32_t i0, uint32_t i1, uint32_t i2);
    };

} // namespace HybridPBR