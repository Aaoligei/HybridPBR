#pragma once
#include <glm/glm.hpp>
#include <cmath>

namespace HybridPBR {

    class BRDF {
    public:
        // 法线分布函数 (GGX/Trowbridge-Reitz)
        static float DistributionGGX(float NdotH, float roughness);
        
        // 几何遮蔽函数 (Smith)
        static float GeometrySchlickGGX(float NdotV, float roughness);
        static float GeometrySmith(float NdotV, float NdotL, float roughness);
        
        // Fresnel方程 (Schlick近似)
        static glm::vec3 FresnelSchlick(float cosTheta, const glm::vec3& F0);
        static glm::vec3 FresnelSchlickRoughness(float cosTheta, const glm::vec3& F0, float roughness);
        
        // 工具函数
        static float RadicalInverse_VdC(uint32_t bits);
        static glm::vec2 Hammersley(uint32_t i, uint32_t N);
        static glm::vec3 ImportanceSampleGGX(glm::vec2 Xi, glm::vec3 N, float roughness);
        
        // PBR光照计算
        static glm::vec3 CalculateDirectLighting(
            const glm::vec3& albedo,
            float metallic,
            float roughness,
            const glm::vec3& N,
            const glm::vec3& V,
            const glm::vec3& L,
            const glm::vec3& lightColor,
            float lightIntensity
        );
    };

} // namespace HybridPBR