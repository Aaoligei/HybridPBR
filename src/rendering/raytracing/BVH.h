#pragma once
#include "Ray.h"
#include "../rasterization/Mesh.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace HybridPBR {

    struct BVHNode {
        AABB bounds;
        int leftChild = -1;
        int firstPrim = 0;
        int primCount = 0;
        
        bool IsLeaf() const { return primCount > 0; }
    };

    struct Triangle {
        glm::vec3 v0, v1, v2;
        glm::vec3 n0, n1, n2;
        glm::vec2 uv0, uv1, uv2;
        uint32_t materialIndex;
        
        AABB GetBounds() const {
            AABB bbox;
            bbox.min = glm::min(v0, glm::min(v1, v2));
            bbox.max = glm::max(v0, glm::max(v1, v2));
            return bbox;
        }
        
        bool Intersect(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
            // Möller–Trumbore 射线-三角形相交算法
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 h = glm::cross(ray.direction, edge2);
            float a = glm::dot(edge1, h);
            
            if (a > -0.000001f && a < 0.000001f) {
                return false; // 射线与三角形平行
            }
            
            float f = 1.0f / a;
            glm::vec3 s = ray.origin - v0;
            float u = f * glm::dot(s, h);
            
            if (u < 0.0f || u > 1.0f) {
                return false;
            }
            
            glm::vec3 q = glm::cross(s, edge1);
            float v = f * glm::dot(ray.direction, q);
            
            if (v < 0.0f || u + v > 1.0f) {
                return false;
            }
            
            float t = f * glm::dot(edge2, q);
            
            if (t > tMin && t < tMax) {
                rec.t = t;
                rec.position = ray.At(t);
                
                // 插值法线
                float w = 1.0f - u - v;
                rec.normal = glm::normalize(n0 * w + n1 * u + n2 * v);
                
                // 插值纹理坐标
                rec.texcoord = uv0 * w + uv1 * u + uv2 * v;
                
                rec.materialIndex = materialIndex;
                rec.SetFaceNormal(ray, rec.normal);
                
                return true;
            }
            
            return false;
        }
    };
    struct GPUMaterial;
    class BVH {
    public:
        BVH();
        ~BVH();
        
        bool Build(const std::vector<std::shared_ptr<Mesh>>& meshes,
                  const std::vector<std::shared_ptr<Material>>& materials);
        bool Intersect(const Ray& ray, float tMin, float tMax, HitRecord& rec) const;
        
        // GPU数据准备
        const std::vector<BVHNode>& GetNodes() const { return nodes; }
        const std::vector<Triangle>& GetTriangles() const { return triangles; }
        const std::vector<GPUMaterial>& GetGPUMaterials() const { return gpuMaterials; }
        
        // 统计信息
        uint32_t GetNodeCount() const { return static_cast<uint32_t>(nodes.size()); }
        uint32_t GetTriangleCount() const { return static_cast<uint32_t>(triangles.size()); }
        uint32_t GetMaxDepth() const { return maxDepth; }

    private:
        std::vector<BVHNode> nodes;
        std::vector<Triangle> triangles;
        std::vector<GPUMaterial> gpuMaterials;
        uint32_t maxDepth = 0;
        
        struct BuildPrimitive {
            AABB bounds;
            glm::vec3 center;
            int triangleIndex;
        };
        
        struct BuildNode {
            AABB bounds;
            int start, end;
        };
        
        // 构建方法
        int BuildRecursive(std::vector<BuildPrimitive>& prims, int start, int end, int depth = 0);
        void CalculateNodeBounds(int nodeIndex, const std::vector<BuildPrimitive>& prims);
        int SplitNode(std::vector<BuildPrimitive>& prims, int start, int end);
        
        // 遍历方法
        bool IntersectRecursive(int nodeIndex, const Ray& ray, float tMin, float tMax, HitRecord& rec) const;
        
        // 工具方法
        void ExtractTrianglesFromMeshes(const std::vector<std::shared_ptr<Mesh>>& meshes,
                                       const std::vector<std::shared_ptr<Material>>& materials);
        void CreateGPUMaterials(const std::vector<std::shared_ptr<Material>>& materials);
    };

    // GPU材质结构（与着色器匹配）
    struct GPUMaterial {
        glm::vec4 albedo;
        glm::vec4 emissive;
        float metallic;
        float roughness;
        float ao;
        uint32_t albedoTexture;
        uint32_t normalTexture;
        uint32_t metallicTexture;
        uint32_t roughnessTexture;
        uint32_t aoTexture;
        uint32_t emissiveTexture;
        
        GPUMaterial() : albedo(0.8f, 0.8f, 0.8f, 1.0f), emissive(0.0f), 
                       metallic(0.0f), roughness(0.5f), ao(1.0f),
                       albedoTexture(0), normalTexture(0), metallicTexture(0),
                       roughnessTexture(0), aoTexture(0), emissiveTexture(0) {}
    };

} // namespace HybridPBR