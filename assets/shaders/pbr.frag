#version 460 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 Bitangent;
    vec3 ViewPos;
} fs_in;

out vec4 FragColor;

// PBR材质
struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    float normalScale;
    vec3 emissive;
    float emissiveIntensity;
    
    bool useAlbedoMap;
    bool useNormalMap;
    bool useMetallicMap;
    bool useRoughnessMap;
    bool useAOMap;
    bool useEmissiveMap;
};

// 光源
// 对应 C++ 的 GPULight
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

// 对应 binding point 1
layout (std140, binding = 1) uniform LightData {
    int lightCount;
    Light lights[16];
};

// 纹理
uniform sampler2D AlbedoMap;
uniform sampler2D NormalMap;
uniform sampler2D MetallicMap;
uniform sampler2D RoughnessMap;
uniform sampler2D AOMap;
uniform sampler2D EmissiveMap;

// IBL纹理
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// 统一变量
uniform Material material;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// 常数
const float PI = 3.14159265359;

// BRDF函数
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
vec3 CalculateNormal();

// 光照计算
vec3 CalculateDirectionalLight(Light light, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness);
vec3 CalculatePointLight(Light light, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness);

void main() {
    // 材质参数
    vec3 albedo = material.albedo;
    float metallic = material.metallic;
    float roughness = material.roughness;
    float ao = material.ao;
    
    if (material.useAlbedoMap) {
        albedo = pow(texture(AlbedoMap, fs_in.TexCoord).rgb, vec3(2.2));
    }
    if (material.useMetallicMap) {
        metallic = texture(MetallicMap, fs_in.TexCoord).r;
    }
    if (material.useRoughnessMap) {
        roughness = texture(RoughnessMap, fs_in.TexCoord).g;
    }
    if (material.useAOMap) {
        ao = texture(AOMap, fs_in.TexCoord).b;
    }
    
    // 输入数据
    vec3 N = CalculateNormal();
    vec3 V = normalize(fs_in.ViewPos - fs_in.FragPos);
    vec3 R = reflect(-V, N);
    
    // 计算反射率
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // 反射方程
    vec3 Lo = vec3(0.0);
    
    //遍历所有光线
    for (int i = 0; i < lightCount; ++i) {
        Light light = lights[i];
        
        if (light.type == 0) { // 方向光
            Lo += CalculateDirectionalLight(light, N, V, F0, albedo, metallic, roughness);
        }else if (light.type == 1) { // 点光
            Lo += CalculatePointLight(light, N, V, F0, albedo, metallic, roughness);
        }else if (light.type == 2) { // spot光
            Lo += CalculatePointLight(light, N, V, F0, albedo, metallic, roughness);
        }
    }
    
    // 环境光贡献 (IBL)
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;
    
    // 采样预滤波贴图和BRDF贴图，然后组合得到镜面IBL部分
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
    
    vec3 ambient = (kD * diffuse + specular) * ao;
    //vec3 ambient = vec3(0.1f);
    
    // 自发光
    vec3 emissive = material.emissive * material.emissiveIntensity;
    if (material.useEmissiveMap) {
        emissive *= texture(EmissiveMap, fs_in.TexCoord).rgb;
    }
    
    vec3 color = ambient + Lo + emissive;
    
    // HDR色调映射
    color = color / (color + vec3(1.0));
    // Gamma校正
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}

// BRDF实现
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalculateNormal() {
    vec3 tangentNormal;
    if(material.useNormalMap)
        tangentNormal = texture(NormalMap, fs_in.TexCoord).xyz * 2.0 - 1.0;
    else
        tangentNormal = fs_in.Normal;
    tangentNormal.xy *= material.normalScale;
    tangentNormal = normalize(tangentNormal);
    
    vec3 Q1 = dFdx(fs_in.FragPos);
    vec3 Q2 = dFdy(fs_in.FragPos);
    vec2 st1 = dFdx(fs_in.TexCoord);
    vec2 st2 = dFdy(fs_in.TexCoord);
    
    vec3 N = normalize(fs_in.Normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

vec3 CalculateDirectionalLight(Light light, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(V + L);
    
    // 光线衰减
    float distance = length(light.direction);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = light.color * light.intensity * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    // 能量守恒
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    // 添加到直接光照
    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 CalculatePointLight(Light light, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 L = normalize(light.position - fs_in.FragPos);
    vec3 H = normalize(V + L);
    float distance = length(light.position - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 radiance = light.color * light.intensity * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}