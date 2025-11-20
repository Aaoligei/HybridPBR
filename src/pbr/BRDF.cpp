#include "BRDF.h"
#include "utils/MathUtils.h"
#include <algorithm>

namespace HybridPBR {

    float BRDF::DistributionGGX(float NdotH, float roughness) {
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH2 = NdotH * NdotH;
        
        float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
        denom = MathUtils::PI * denom * denom;
        
        return a2 / std::max(denom, 0.0000001f);
    }

    float BRDF::GeometrySchlickGGX(float NdotV, float roughness) {
        float r = (roughness + 1.0f);
        float k = (r * r) / 8.0f;
        
        float denom = NdotV * (1.0f - k) + k;
        return NdotV / denom;
    }

    float BRDF::GeometrySmith(float NdotV, float NdotL, float roughness) {
        float ggx2 = GeometrySchlickGGX(NdotV, roughness);
        float ggx1 = GeometrySchlickGGX(NdotL, roughness);
        return ggx1 * ggx2;
    }

    glm::vec3 BRDF::FresnelSchlick(float cosTheta, const glm::vec3& F0) {
        return F0 + (1.0f - F0) * std::pow(std::max(1.0f - cosTheta, 0.0f), 5.0f);
    }

    glm::vec3 BRDF::FresnelSchlickRoughness(float cosTheta, const glm::vec3& F0, float roughness) {
        // 使用glm::max而不是std::max来处理向量比较
        return F0 + (glm::max(glm::vec3(1.0f - roughness), F0) - F0) * 
               std::pow(std::max(1.0f - cosTheta, 0.0f), 5.0f);
    }

    float BRDF::RadicalInverse_VdC(uint32_t bits) {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return float(bits) * 2.3283064365386963e-10f;
    }

    glm::vec2 BRDF::Hammersley(uint32_t i, uint32_t N) {
        return glm::vec2(float(i) / float(N), RadicalInverse_VdC(i));
    }

    glm::vec3 BRDF::ImportanceSampleGGX(glm::vec2 Xi, glm::vec3 N, float roughness) {
        float a = roughness * roughness;
        
        float phi = 2.0f * MathUtils::PI * Xi.x;
        float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
        float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
        
        // 从球面坐标转换为笛卡尔坐标
        glm::vec3 H;
        H.x = cos(phi) * sinTheta;
        H.y = sin(phi) * sinTheta;
        H.z = cosTheta;
        
        // 从切线空间转换为世界空间
        glm::vec3 up = abs(N.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 tangent = normalize(cross(up, N));
        glm::vec3 bitangent = cross(N, tangent);
        
        return tangent * H.x + bitangent * H.y + N * H.z;
    }

    glm::vec3 BRDF::CalculateDirectLighting(
        const glm::vec3& albedo,
        float metallic,
        float roughness,
        const glm::vec3& N,
        const glm::vec3& V,
        const glm::vec3& L,
        const glm::vec3& lightColor,
        float lightIntensity) {
        
        glm::vec3 H = normalize(V + L);
        
        // 计算基础参数
        float NdotV = std::max(dot(N, V), 0.0f);
        float NdotL = std::max(dot(N, L), 0.0f);
        float NdotH = std::max(dot(N, H), 0.0f);
        float HdotV = std::max(dot(H, V), 0.0f);
        
        // 基础反射率
        glm::vec3 F0 = glm::vec3(0.04f);
        F0 = mix(F0, albedo, metallic);
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(NdotH, roughness);
        float G = GeometrySmith(NdotV, NdotL, roughness);
        glm::vec3 F = FresnelSchlick(HdotV, F0);
        
        // 镜面反射部分
        glm::vec3 numerator = NDF * G * F;
        float denominator = 4.0f * NdotV * NdotL + 0.0001f;
        glm::vec3 specular = numerator / denominator;
        
        // 漫反射部分 (能量守恒)
        glm::vec3 kS = F;
        glm::vec3 kD = (glm::vec3(1.0f) - kS) * (1.0f - metallic);
        
        // 添加入射光
        glm::vec3 radiance = lightColor * lightIntensity * NdotL;
        
        return (kD * albedo / glm::pi<float>() + specular) * radiance;
    }

} // namespace HybridPBR