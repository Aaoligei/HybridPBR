#pragma once
#include <glm/glm.hpp>

namespace HybridPBR {

#pragma pack(push, 16)
    struct Ray {
        glm::vec3 origin;
        float pad0;
        glm::vec3 direction;
        float pad1;
        float tMin;
        float tMax;
        float pad2, pad3; // 填充至32字节对齐
        
        Ray() : origin(0.0f), direction(0.0f, 0.0f, 1.0f), tMin(0.001f), tMax(10000.0f) {}
        Ray(const glm::vec3& o, const glm::vec3& d, float minT = 0.001f, float maxT = 10000.0f)
            : origin(o), direction(d), tMin(minT), tMax(maxT) {}
        
        glm::vec3 At(float t) const {
            return origin + direction * t;
        }
    };

    struct HitRecord {
        glm::vec3 position;
        float pad0;
        glm::vec3 normal;
        float pad1;
        glm::vec2 texcoord;
        float pad2, pad3;
        float t = -1.0f;
        uint32_t materialIndex = 0;
        uint32_t triangleIndex = 0;
        bool frontFace = true;
        uint32_t pad4; // 填充至48字节对齐
        
        void SetFaceNormal(const Ray& ray, const glm::vec3& outwardNormal) {
            frontFace = glm::dot(ray.direction, outwardNormal) < 0;
            normal = frontFace ? outwardNormal : -outwardNormal;
        }
    };

    struct AABB {
        glm::vec3 min;
        float pad0;
        glm::vec3 max;
        float pad1;
        
        AABB() : min(FLT_MAX), max(-FLT_MAX) {}
        AABB(const glm::vec3& a, const glm::vec3& b) : min(a), max(b) {}
        
        bool Intersect(const Ray& ray, float tMin, float tMax) const {
            for (int i = 0; i < 3; i++) {
                float invD = 1.0f / ray.direction[i];
                float t0 = (min[i] - ray.origin[i]) * invD;
                float t1 = (max[i] - ray.origin[i]) * invD;
                
                if (invD < 0.0f) {
                    std::swap(t0, t1);
                }
                
                tMin = t0 > tMin ? t0 : tMin;
                tMax = t1 < tMax ? t1 : tMax;
                
                if (tMax <= tMin) {
                    return false;
                }
            }
            return true;
        }
        
        glm::vec3 Center() const {
            return (min + max) * 0.5f;
        }
        
        float SurfaceArea() const {
            glm::vec3 extent = max - min;
            return 2.0f * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
        }
    };
#pragma pack(pop)

} // namespace HybridPBR