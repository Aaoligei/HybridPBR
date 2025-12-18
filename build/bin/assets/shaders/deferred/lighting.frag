#version 450 core
out vec4 FragColor;

in vec2 TexCoords;

// G-Buffer Samplers
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMRA;
uniform sampler2D gEmissive;

// IBL Samplers
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform bool useIBL;

// Hybrid Ray Tracing Maps (来自 C++ SetHybridMaps)
uniform sampler2D rtShadowMap;
uniform sampler2D rtReflectionMap;
uniform bool useRTShadows;
uniform bool useRTReflections;

// Uniform Buffers
// Binding 0: Camera
layout (std140, binding = 2) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec3 viewPos;
};

// 光源结构体定义 (必须与 C++ 的 LightData 内存布局严格一致)
// C++ vec3 通常是 12 字节，但在 std140 中 vec3 是 16 字节对齐
struct Light {
    vec3 position;  
    // padding ...
    vec3 direction; 
    // padding ...
    vec3 color;     
    float intensity;

    float range;
    float constant;
    float linear;
    float quadratic;

    float innerCutoff;
    float outerCutoff;
    int type;
    // padding ...
};


layout (std140, binding = 3) uniform LightData {
    int lightCount;
    Light lights[16];
};

const float PI = 3.14159265359;

// --- PBR 辅助函数 (Fresnel, Geometry, Distribution) ---

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / max(denom, 0.001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {

    // 1. 从 G-Buffer 采样数据
    vec3 WorldPos = texture(gPosition, TexCoords).rgb;
    // 如果 WorldPos 是 0 (例如背景)，则丢弃或仅渲染天空盒
    // 在 Deferred 中，通常通过深度判断，这里简单处理
    float depth = texture(gPosition, TexCoords).a; // 假设 Alpha 没存东西，或者读取 DepthBuffer
    
    vec3 N = normalize(texture(gNormal, TexCoords).rgb);
    vec3 Albedo = texture(gAlbedo, TexCoords).rgb;
    vec4 MRA = texture(gMRA, TexCoords);
    float Metallic = MRA.r;
    float Roughness = MRA.g;
    float AO = MRA.b;
    vec3 Emissive = texture(gEmissive, TexCoords).rgb;

    vec3 V = normalize(viewPos - WorldPos);
    vec3 R = reflect(-V, N);

    // F0: 基础反射率，非金属 0.04，金属使用 Albedo
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, Albedo, Metallic);

    vec3 Lo = vec3(0.0);

    // 2. 遍历光源计算直接光照
    for(int i = 0; i < lightCount; ++i) {
        Light light = lights[i];
        
        vec3 L;
        float attenuation = 1.0;

        // 计算光照方向和衰减
        if (light.type == 0) { // Directional
            L = normalize(-light.direction);
        } else { // Point or Spot
            L = normalize(light.position - WorldPos);
            float distance = length(light.position - WorldPos);
            
            if (distance > light.range) continue; // 简单剔除

            // 物理正确的平方反比衰减
            attenuation = 1.0 / (distance * distance);
            // 或者使用 constants/linear/quad 参数
            // attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
            
            if (light.type == 2) { // Spot
                float theta = dot(L, normalize(-light.direction));
                float epsilon = light.innerCutoff - light.outerCutoff;
                float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
                attenuation *= intensity;
            }
        }

        vec3 radiance = light.color * light.intensity * attenuation;

        // --- Cook-Torrance BRDF ---
        vec3 H = normalize(V + L);
        
        float NDF = DistributionGGX(N, H, Roughness);   
        float G   = GeometrySmith(N, V, L, Roughness);    
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - Metallic;	  

        float NdotL = max(dot(N, L), 0.0);        

        // 阴影计算 (Hybrid Ray Tracing)
        float shadow = 1.0;
        if (useRTShadows) {
            // 假设 rtShadowMap 存储的是可见性 (1=亮, 0=影)
            // 需要根据屏幕空间坐标采样
            shadow = texture(rtShadowMap, TexCoords).r;
        }

        Lo += (kD * Albedo / PI + specular) * radiance * NdotL * shadow; 
    }

    // 3. 环境光照 (IBL)
    vec3 ambient = vec3(0.03) * Albedo * AO; // Fallback

    if (useIBL) {
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, Roughness);
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - Metallic;
        
        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuse = irradiance * Albedo;
        
        // 采样预滤波环境图
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, Roughness * MAX_REFLECTION_LOD).rgb;    
        vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), Roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

        // 如果开启了光追反射，混合 RT 反射和 IBL 反射
        if (useRTReflections) {
            vec3 rtReflect = texture(rtReflectionMap, TexCoords).rgb;
            // 简单的混合策略：越光滑越倾向于使用 RT 结果
            // 或者 rtReflectionMap 包含 alpha 用于混合
            specular = mix(specular, rtReflect, 0.5); // 这里仅作示例
        }

        ambient = (kD * diffuse + specular) * AO;
    }

    // 4. 合成最终颜色
    vec3 color = ambient + Lo + Emissive;

    // HDR Tonemapping & Gamma Correct (如果你没有单独的 PostProcessPass，可以在这里做)
    // color = color / (color + vec3(1.0));
    // color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}