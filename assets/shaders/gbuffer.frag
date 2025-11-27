#version 460 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 Bitangent;
} fs_in;

// G-Buffer输出
layout (location = 0) out vec4 gPosition;     // 世界空间位置
layout (location = 1) out vec4 gNormal;       // 世界空间法线
layout (location = 2) out vec4 gAlbedo;       // 基础色
layout (location = 3) out vec4 gPbrParams;    // 金属度(R), 粗糙度(G), AO(B)

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

// 纹理
uniform sampler2D AlbedoMap;
uniform sampler2D NormalMap;
uniform sampler2D MetallicMap;
uniform sampler2D RoughnessMap;
uniform sampler2D AOMap;
uniform sampler2D EmissiveMap;

// 统一变量
uniform Material material;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

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

void main() {
    // 计算世界空间法线
    vec3 worldNormal = CalculateNormal();
    
    // 获取基础色
    vec3 albedo = material.albedo;
    if (material.useAlbedoMap) {
        albedo = pow(texture(AlbedoMap, fs_in.TexCoord).rgb, vec3(2.2));
    }
    
    // 获取金属度
    float metallic = material.metallic;
    if (material.useMetallicMap) {
        metallic = texture(MetallicMap, fs_in.TexCoord).r;
    }
    
    // 获取粗糙度
    float roughness = material.roughness;
    if (material.useRoughnessMap) {
        roughness = texture(RoughnessMap, fs_in.TexCoord).g;
    }
    
    // 获取AO
    float ao = material.ao;
    if (material.useAOMap) {
        ao = texture(AOMap, fs_in.TexCoord).b;
    }
    
    // 输出到G-Buffer
    gPosition = vec4(fs_in.FragPos, 1.0);
    gNormal = vec4(worldNormal, 1.0);
    gAlbedo = vec4(albedo, 1.0);
    gPbrParams = vec4(metallic, roughness, ao, 1.0);
}