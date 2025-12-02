#include "Mesh.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cfloat>
#include "utils/Logger.h"

namespace HybridPBR {

    Mesh::Mesh(const std::string& meshName)
        : name(meshName), VAO(0), VBO(0), EBO(0), buffersInitialized(false) {
        minBounds = glm::vec3(FLT_MAX);
        maxBounds = glm::vec3(-FLT_MAX);
    }

    Mesh::~Mesh() {
        if (VAO) {
            glDeleteVertexArrays(1, &VAO);
        }
        if (VBO) {
            glDeleteBuffers(1, &VBO);
        }
        if (EBO) {
            glDeleteBuffers(1, &EBO);
        }
    }

    void Mesh::SetVertices(const std::vector<Vertex>& newVertices) {
        vertices = newVertices;
        UpdateBounds();
        buffersInitialized = false;
    }

    void Mesh::SetIndices(const std::vector<uint32_t>& newIndices) {
        indices = newIndices;
        buffersInitialized = false;
    }

    void Mesh::CalculateNormals() {
        if (vertices.size() < 3) return;

        // 重置法线
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

        // 标准化法线
        for (auto& vertex : vertices) {
            vertex.normal = glm::normalize(vertex.normal);
        }

        buffersInitialized = false;
    }

    void Mesh::CalculateTangents() {
        if (vertices.size() < 3) return;

        // 重置切线和副切线
        for (auto& vertex : vertices) {
            vertex.tangent = glm::vec3(0.0f);
            vertex.bitangent = glm::vec3(0.0f);
        }

        // 计算每个面的切线空间并累加到顶点
        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            // 获取三角形的顶点位置和纹理坐标
            glm::vec3 pos0 = vertices[i0].position;
            glm::vec3 pos1 = vertices[i1].position;
            glm::vec3 pos2 = vertices[i2].position;
            
            glm::vec2 uv0 = vertices[i0].texcoord;
            glm::vec2 uv1 = vertices[i1].texcoord;
            glm::vec2 uv2 = vertices[i2].texcoord;

            // 计算边
            glm::vec3 edge1 = pos1 - pos0;
            glm::vec3 edge2 = pos2 - pos0;
            
            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            glm::vec3 tangent = glm::vec3(0.0f);
            glm::vec3 bitangent = glm::vec3(0.0f);
            
            if (abs(deltaUV1.x) > 0.0001f || abs(deltaUV2.x) > 0.0001f || 
                abs(deltaUV1.y) > 0.0001f || abs(deltaUV2.y) > 0.0001f) {
                tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
                bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);
            } else {
                // 如果UV坐标无效，则使用默认切线空间
                tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            // 累加到顶点
            vertices[i0].tangent += tangent;
            vertices[i1].tangent += tangent;
            vertices[i2].tangent += tangent;
            
            vertices[i0].bitangent += bitangent;
            vertices[i1].bitangent += bitangent;
            vertices[i2].bitangent += bitangent;
        }

        // 标准化切线和副切线，并使用Gram-Schmidt正交化
        for (auto& vertex : vertices) {
            // Gram-Schmidt正交化以确保切线与法线垂直
            vertex.tangent = glm::normalize(vertex.tangent - glm::dot(vertex.tangent, vertex.normal) * vertex.normal);
            
            // 重新计算副切线以确保正确的手性
            vertex.bitangent = glm::normalize(glm::cross(vertex.normal, vertex.tangent));
        }

        buffersInitialized = false;
    }

    void Mesh::GeneratePlane(float width, float height, uint32_t subdivisions) {
        vertices.clear();
        indices.clear();

        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;
        
        float stepX = width / static_cast<float>(subdivisions);
        float stepY = height / static_cast<float>(subdivisions);
        
        float stepU = 1.0f / static_cast<float>(subdivisions);
        float stepV = 1.0f / static_cast<float>(subdivisions);

        // 生成顶点
        for (uint32_t y = 0; y <= subdivisions; ++y) {
            for (uint32_t x = 0; x <= subdivisions; ++x) {
                float xPos = -halfWidth + x * stepX;
                float yPos = -halfHeight + y * stepY;
                
                Vertex vertex;
                vertex.position = glm::vec3(xPos, yPos, 0.0f);
                vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);
                vertex.texcoord = glm::vec2(x * stepU, 1.0f - y * stepV); // 翻转V坐标
                vertex.tangent = glm::vec3(0.0f);
                vertex.bitangent = glm::vec3(0.0f);
                
                vertices.push_back(vertex);
            }
        }

        // 生成索引
        for (uint32_t y = 0; y < subdivisions; ++y) {
            for (uint32_t x = 0; x < subdivisions; ++x) {
                uint32_t topLeft = y * (subdivisions + 1) + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (y + 1) * (subdivisions + 1) + x;
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

        CalculateTangents();
        UpdateBounds();
        buffersInitialized = false;
    }

    void Mesh::GenerateCube(float size) {
        vertices.clear();
        indices.clear();

        float halfSize = size * 0.5f;
        
        // 定义立方体的8个顶点
        glm::vec3 positions[8] = {
            glm::vec3(-halfSize, -halfSize, -halfSize), // 0
            glm::vec3( halfSize, -halfSize, -halfSize), // 1
            glm::vec3( halfSize,  halfSize, -halfSize), // 2
            glm::vec3(-halfSize,  halfSize, -halfSize), // 3
            glm::vec3(-halfSize, -halfSize,  halfSize), // 4
            glm::vec3( halfSize, -halfSize,  halfSize), // 5
            glm::vec3( halfSize,  halfSize,  halfSize), // 6
            glm::vec3(-halfSize,  halfSize,  halfSize)  // 7
        };

        // 定义每个面的顶点索引和法线
        struct Face {
            uint32_t indices[4];
            glm::vec3 normal;
            glm::vec2 uvs[4];
        };

        Face faces[6] = {
            // 前面 (Z-)
            {{0, 1, 2, 3}, glm::vec3(0.0f, 0.0f, -1.0f), {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)}},
            // 后面 (Z+)
            {{5, 4, 7, 6}, glm::vec3(0.0f, 0.0f, 1.0f), {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)}},
            // 左面 (X-)
            {{4, 0, 3, 7}, glm::vec3(-1.0f, 0.0f, 0.0f), {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)}},
            // 右面 (X+)
            {{1, 5, 6, 2}, glm::vec3(1.0f, 0.0f, 0.0f), {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)}},
            // 底面 (Y-)
            {{0, 4, 5, 1}, glm::vec3(0.0f, -1.0f, 0.0f), {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)}},
            // 顶面 (Y+)
            {{3, 2, 6, 7}, glm::vec3(0.0f, 1.0f, 0.0f), {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)}}
        };

        // 为每个面生成顶点和索引
        for (int face = 0; face < 6; ++face) {
            uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
            
            // 添加4个顶点
            for (int i = 0; i < 4; ++i) {
                Vertex vertex;
                vertex.position = positions[faces[face].indices[i]];
                vertex.normal = faces[face].normal;
                vertex.texcoord = faces[face].uvs[i];
                vertex.tangent = glm::vec3(0.0f);
                vertex.bitangent = glm::vec3(0.0f);
                vertices.push_back(vertex);
            }

            // 添加索引（两个三角形）
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 1);
            
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 3);
            indices.push_back(baseIndex + 2);
        }

        CalculateTangents();
        UpdateBounds();
        buffersInitialized = false;
    }

    void Mesh::GenerateSphere(float radius, uint32_t segments) {
        vertices.clear();
        indices.clear();

        // 生成顶点
        for (uint32_t y = 0; y <= segments; ++y) {
            for (uint32_t x = 0; x <= segments; ++x) {
                float xSegment = static_cast<float>(x) / static_cast<float>(segments);
                float ySegment = static_cast<float>(y) / static_cast<float>(segments);
                
                float xPos = std::cos(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
                float yPos = std::cos(ySegment * glm::pi<float>());
                float zPos = std::sin(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());

                Vertex vertex;
                vertex.position = glm::vec3(xPos, yPos, zPos) * radius;
                // 初始法线设置为位置的标准化向量（球体的正确法线方向）
                vertex.normal = glm::normalize(glm::vec3(xPos, yPos, zPos));
                vertex.texcoord = glm::vec2(xSegment, ySegment);
                vertex.tangent = glm::vec3(0.0f);
                vertex.bitangent = glm::vec3(0.0f);
                
                vertices.push_back(vertex);
            }
        }

        // 生成索引
        for (uint32_t y = 0; y < segments; ++y) {
            for (uint32_t x = 0; x < segments; ++x) {
                uint32_t first = y * (segments + 1) + x;
                uint32_t second = first + segments + 1;
                
                indices.push_back(first);
                indices.push_back(first + 1);
                indices.push_back(second);
                
                indices.push_back(second);
                indices.push_back(first + 1);
                indices.push_back(second + 1);
            }
        }

        // 重新计算法线以确保准确性（使用相邻面的法线平均）
        //CalculateNormals();
        CalculateTangents();
        UpdateBounds();
        buffersInitialized = false;
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
    }

    void Mesh::RenderInstanced(uint32_t instanceCount) const {
        if (!IsValid() || instanceCount == 0) return;

        if (!buffersInitialized) {
            const_cast<Mesh*>(this)->SetupBuffers();
        }

        glBindVertexArray(VAO);
        
        if (HasIndices()) {
            glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0, instanceCount);
        } else {
            glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()), instanceCount);
        }
        
        glBindVertexArray(0);
    }

    bool Mesh::IsValid() const {
        return !vertices.empty();
    }

    float Mesh::GetBoundingRadius() const {
        glm::vec3 extent = (maxBounds - minBounds) * 0.5f;
        return glm::length(extent);
    }

    void Mesh::SetupBuffers() {
        if (VAO) {
            glDeleteVertexArrays(1, &VAO);
        }
        if (VBO) {
            glDeleteBuffers(1, &VBO);
        }
        if (EBO) {
            glDeleteBuffers(1, &EBO);
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);
        
        // 法线属性
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);
        
        // 纹理坐标属性
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
        glEnableVertexAttribArray(2);
        
        // 切线属性
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(3);
        
        // 副切线属性
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
        glEnableVertexAttribArray(4);

        if (!indices.empty()) {
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
        buffersInitialized = true;
    }

    void Mesh::UpdateBounds() {
        minBounds = glm::vec3(FLT_MAX);
        maxBounds = glm::vec3(-FLT_MAX);

        for (const auto& vertex : vertices) {
            minBounds = glm::min(minBounds, vertex.position);
            maxBounds = glm::max(maxBounds, vertex.position);
        }
    }

    void Mesh::CalculateNormal(uint32_t i0, uint32_t i1, uint32_t i2) {
        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            return;
        }

        glm::vec3 v0 = vertices[i0].position;
        glm::vec3 v1 = vertices[i1].position;
        glm::vec3 v2 = vertices[i2].position;

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        vertices[i0].normal += normal;
        vertices[i1].normal += normal;
        vertices[i2].normal += normal;
    }

} // namespace HybridPBR