#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out vec4 gMRA;
layout (location = 4) out vec3 gEmissive;

in VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

// 定义与 C++ PBRMaterial::ApplyToShader 匹配的结构体
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec3 emissive;      // 这里的 emissive 对应 C++ 的 emissiveColor
    float emissiveIntensity;
    float normalScale;

    // 对应 C++ 的 SetBool("material.use...Map")
    bool useAlbedoMap;
    bool useNormalMap;
    bool useMetallicMap;
    bool useRoughnessMap;
    bool useAOMap;
    bool useEmissiveMap;
};

uniform Material material;

// 对应 C++ 的 SetInt("...Map")，注意大小写匹配
uniform sampler2D AlbedoMap;
uniform sampler2D NormalMap;
uniform sampler2D MetallicMap;
uniform sampler2D RoughnessMap;
uniform sampler2D AOMap;
uniform sampler2D EmissiveMap;

void main() {
    // 1. Position
    gPosition = fs_in.WorldPos;

    // 2. Normal
    vec3 N = normalize(fs_in.Normal);
    if (material.useNormalMap) {
        vec3 normalMapValue = texture(NormalMap, fs_in.TexCoords).rgb;
        normalMapValue = normalMapValue * 2.0 - 1.0;
        // 应用法线强度
        normalMapValue.xy *= material.normalScale; 
        N = normalize(fs_in.TBN * normalMapValue);
    }
    gNormal = N;

    // 3. Albedo
    vec3 Albedo = albedo;
    if (material.useAlbedoMap) {
        // 假设纹理是 sRGB，这里需要线性化吗？取决于你的 Texture 加载参数
        // 如果 Texture Load 设置了 GL_SRGB，这里采样出来已经是线性的
        Albedo = texture(AlbedoMap, fs_in.TexCoords).rgb;
    }
    gAlbedo = vec4(Albedo, 1.0);

    // 4. MRA
    float Metallic = metallic;
    if (material.useMetallicMap) Metallic = texture(MetallicMap, fs_in.TexCoords).r;

    float Roughness = roughness;
    if (material.useRoughnessMap) Roughness = texture(RoughnessMap, fs_in.TexCoords).r;

    float Ao = ao;
    if (material.useAOMap) Ao = texture(AOMap, fs_in.TexCoords).r;

    gMRA = vec4(Metallic, Roughness, Ao, 1.0);

    // 5. Emissive
    vec3 emissive = material.emissive * material.emissiveIntensity;
    if (material.useEmissiveMap) {
        emissive *= texture(EmissiveMap, fs_in.TexCoords).rgb;
    }
    gEmissive = emissive;
}