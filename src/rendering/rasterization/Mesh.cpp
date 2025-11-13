#include "Mesh.h"
#include "utils/MathUtils.h"

namespace HybridPBR {

    Mesh::Mesh(const std::string& meshName) 
        : name(meshName) {
    }

    Mesh::~Mesh() {
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            if (EBO != 0) {
                glDeleteBuffers(1, &EBO);
            }
        }
    }

    void Mesh::SetVertices(const std::vector<Vertex>& newVertices) {
        vertices = newVertices;
        UpdateBounds();
        
        if (buffersInitialized) {
            SetupBuffers();
        }
    }

    void Mesh::SetIndices(const std::vector<uint32_t>& newIndices) {
        indices = newIndices;
        
        if (buffersInitialized) {
            SetupBuffers();
        }
    }

    void Mesh::CalculateNormals() {
        // 如果已经有法线，先清零
        for (auto& vertex : vertices) {
            vertex.normal = glm::vec3(0.0f);
        }
        
        // 计算每个面的法线并累加到顶点
        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            
            CalculateNormal(i0, i1, i2);
        }
        
        // 归一化所有法线
        for (auto& vertex : vertices) {
            vertex.normal = glm::normalize(vertex.normal);
        }
    }

    void Mesh::CalculateTangents() {
        // 实现切线计算
        // 这里简化处理，实际需要根据纹理坐标计算
        for (auto& vertex : vertices) {
            vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            vertex.bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    void Mesh::GeneratePlane(float width, float height, uint32_t subdivisions) {
        vertices.clear();
        indices.clear();
        
        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;
        
        uint32_t xSegments = subdivisions + 1;
        uint32_t ySegments = subdivisions + 1;
        
        // 生成顶点
        for (uint32_t y = 0; y < ySegments; ++y) {
            for (uint32_t x = 0; x < xSegments; ++x) {
                Vertex vertex;
                float u = static_cast<float>(x) / (xSegments - 1);
                float v = static_cast<float>(y) / (ySegments - 1);
                
                vertex.position.x = (u - 0.5f) * width;
                vertex.position.y = 0.0f;
                vertex.position.z = (v - 0.5f) * height;
                
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vertex.texcoord = glm::vec2(u, v);
                
                vertices.push_back(vertex);
            }
        }
        
        // 生成索引
        for (uint32_t y = 0; y < subdivisions; ++y) {
            for (uint32_t x = 0; x < subdivisions; ++x) {
                uint32_t topLeft = y * xSegments + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (y + 1) * xSegments + x;
                uint32_t bottomRight = bottomLeft + 1;
                
                // 第一个三角形
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);
                
                // 第二个三角形
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
        
        UpdateBounds();
    }

    void Mesh::GenerateCube(float size) {
        vertices.clear();
        indices.clear();
        
        float halfSize = size * 0.5f;
        
        // 定义立方体的8个顶点
        std::vector<glm::vec3> positions = {
            // 前面
            {-halfSize, -halfSize,  halfSize}, { halfSize, -halfSize,  halfSize},
            { halfSize,  halfSize,  halfSize}, {-halfSize,  halfSize,  halfSize},
            // 后面
            {-halfSize, -halfSize, -halfSize}, {-halfSize,  halfSize, -halfSize},
            { halfSize,  halfSize, -halfSize}, { halfSize, -halfSize, -halfSize},
            // 上面
            {-halfSize,  halfSize,  halfSize}, { halfSize,  halfSize,  halfSize},
            { halfSize,  halfSize, -halfSize}, {-halfSize,  halfSize, -halfSize},
            // 下面
            {-halfSize, -halfSize,  halfSize}, {-halfSize, -halfSize, -halfSize},
            { halfSize, -halfSize, -halfSize}, { halfSize, -halfSize,  halfSize},
            // 右面
            { halfSize, -halfSize,  halfSize}, { halfSize, -halfSize, -halfSize},
            { halfSize,  halfSize, -halfSize}, { halfSize,  halfSize,  halfSize},
            // 左面
            {-halfSize, -halfSize,  halfSize}, {-halfSize,  halfSize,  halfSize},
            {-halfSize,  halfSize, -halfSize}, {-halfSize, -halfSize, -halfSize}
        };
        
        std::vector<glm::vec3> normals = {
            // 前面
            {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1},
            // 后面
            {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
            // 上面
            {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0},
            // 下面
            {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0},
            // 右面
            {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0},
            // 左面
            {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}
        };
        
        std::vector<glm::vec2> texcoords = {
            {0, 0}, {1, 0}, {1, 1}, {0, 1},
            {0, 0}, {1, 0}, {1, 1}, {0, 1},
            {0, 0}, {1, 0}, {1, 1}, {0, 1},
            {0, 0}, {1, 0}, {1, 1}, {0, 1},
            {0, 0}, {1, 0}, {1, 1}, {0, 1},
            {0, 0}, {1, 0}, {1, 1}, {0, 1}
        };
        
        // 创建顶点
        for (size_t i = 0; i < positions.size(); ++i) {
            Vertex vertex;
            vertex.position = positions[i];
            vertex.normal = normals[i];
            vertex.texcoord = texcoords[i];
            vertices.push_back(vertex);
        }
        
        // 定义索引（每个面2个三角形）
        std::vector<uint32_t> cubeIndices = {
            // 前面
            0, 1, 2, 2, 3, 0,
            // 后面
            4, 5, 6, 6, 7, 4,
            // 上面
            8, 9, 10, 10, 11, 8,
            // 下面
            12, 13, 14, 14, 15, 12,
            // 右面
            16, 17, 18, 18, 19, 16,
            // 左面
            20, 21, 22, 22, 23, 20
        };
        
        indices = cubeIndices;
        UpdateBounds();
    }

    void Mesh::GenerateSphere(float radius, uint32_t segments) {
        vertices.clear();
        indices.clear();
        
        // 生成球体顶点
        for (uint32_t y = 0; y <= segments; ++y) {
            for (uint32_t x = 0; x <= segments; ++x) {
                Vertex vertex;
                
                float xSegment = static_cast<float>(x) / segments;
                float ySegment = static_cast<float>(y) / segments;
                
                float xPos = std::cos(xSegment * 2.0f * MathUtils::PI) * std::sin(ySegment * MathUtils::PI);
                float yPos = std::cos(ySegment * MathUtils::PI);
                float zPos = std::sin(xSegment * 2.0f * MathUtils::PI) * std::sin(ySegment * MathUtils::PI);
                
                vertex.position = glm::vec3(xPos, yPos, zPos) * radius;
                vertex.normal = glm::normalize(vertex.position);
                vertex.texcoord = glm::vec2(xSegment, ySegment);
                
                vertices.push_back(vertex);
            }
        }
        
        // 生成索引
        for (uint32_t y = 0; y < segments; ++y) {
            for (uint32_t x = 0; x < segments; ++x) {
                uint32_t first = y * (segments + 1) + x;
                uint32_t second = first + segments + 1;
                
                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);
                
                indices.push_back(first + 1);
                indices.push_back(second);
                indices.push_back(second + 1);
            }
        }
        
        UpdateBounds();
    }

    void Mesh::Render() const {
        if (!IsValid()) return;
        
        if (!buffersInitialized) {
            const_cast<Mesh*>(this)->SetupBuffers();
        }
        
        glBindVertexArray(VAO);
        
        if (HasIndices()) {
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        }
        
        glBindVertexArray(0);
        
        // 更新统计
        // 注意：这里需要访问非const的stats，但在实际渲染器中处理
    }

    void Mesh::RenderInstanced(uint32_t instanceCount) const {
        if (!IsValid()) return;
        
        if (!buffersInitialized) {
            const_cast<Mesh*>(this)->SetupBuffers();
        }
        
        glBindVertexArray(VAO);
        
        if (HasIndices()) {
            glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), 
                                   GL_UNSIGNED_INT, 0, instanceCount);
        } else {
            glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()), instanceCount);
        }
        
        glBindVertexArray(0);
    }

    bool Mesh::IsValid() const {
        return !vertices.empty();
    }

    float Mesh::GetBoundingRadius() const {
        glm::vec3 center = GetCenter();
        float maxDistance = 0.0f;
        
        for (const auto& vertex : vertices) {
            float distance = glm::length(vertex.position - center);
            if (distance > maxDistance) {
                maxDistance = distance;
            }
        }
        
        return maxDistance;
    }

    void Mesh::SetupBuffers() {
        if (VAO == 0) {
            glGenVertexArrays(1, &VAO);
        }
        
        glBindVertexArray(VAO);
        
        // 顶点缓冲区
        if (VBO == 0) {
            glGenBuffers(1, &VBO);
        }
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), 
                    vertices.data(), GL_STATIC_DRAW);
        
        // 索引缓冲区
        if (!indices.empty()) {
            if (EBO == 0) {
                glGenBuffers(1, &EBO);
            }
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
                        indices.data(), GL_STATIC_DRAW);
        }
        
        // 顶点属性
        // 位置
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        
        // 法线
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        
        // 纹理坐标
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
        
        // 切线
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
        
        // 副切线
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
        
        glBindVertexArray(0);
        buffersInitialized = true;
    }

    void Mesh::UpdateBounds() {
        if (vertices.empty()) {
            minBounds = glm::vec3(0.0f);
            maxBounds = glm::vec3(0.0f);
            return;
        }
        
        minBounds = glm::vec3(FLT_MAX);
        maxBounds = glm::vec3(-FLT_MAX);
        
        for (const auto& vertex : vertices) {
            minBounds = glm::min(minBounds, vertex.position);
            maxBounds = glm::max(maxBounds, vertex.position);
        }
    }

    void Mesh::CalculateNormal(uint32_t i0, uint32_t i1, uint32_t i2) {
        glm::vec3 v0 = vertices[i0].position;
        glm::vec3 v1 = vertices[i1].position;
        glm::vec3 v2 = vertices[i2].position;
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        vertices[i0].normal += normal;
        vertices[i1].normal += normal;
        vertices[i2].normal += normal;
    }

} // namespace HybridPBR