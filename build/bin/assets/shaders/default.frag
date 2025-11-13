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

// 材质属性
struct Material {
    vec4 albedo;
    float metallic;
    float roughness;
    float ambientOcclusion;
    float normalScale;
    vec3 emissiveColor;
    float emissiveIntensity;
    vec2 textureScale;
    vec2 textureOffset;
    
    // 纹理使用标志
    bool useAlbedoMap;
    bool useNormalMap;
    bool useMetallicMap;
    bool useRoughnessMap;
    bool useAOMap;
    bool useEmissiveMap;
};

// 方向光
struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

uniform Material material;
uniform DirectionalLight directionalLight;
uniform int directionalLightCount;
uniform vec3 ambientLight;

// 纹理采样器
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D emissiveMap;

// 工具函数
vec3 CalculateNormal();
vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir);
vec3 CalculateAmbientLight(vec3 albedo);

void main() {
    // 应用纹理缩放和偏移
    vec2 texCoord = fs_in.TexCoord * material.textureScale + material.textureOffset;
    
    // 获取基础颜色
    vec3 albedo = material.albedo.rgb;
    if (material.useAlbedoMap) {
        albedo = texture(albedoMap, texCoord).rgb;
    }
    
    // 获取法线
    vec3 normal = CalculateNormal();
    
    // 获取其他材质参数
    float metallic = material.metallic;
    float roughness = material.roughness;
    float ao = material.ambientOcclusion;
    
    if (material.useMetallicMap) {
        metallic = texture(metallicMap, texCoord).r;
    }
    if (material.useRoughnessMap) {
        roughness = texture(roughnessMap, texCoord).r;
    }
    if (material.useAOMap) {
        ao = texture(aoMap, texCoord).r;
    }
    
    // 计算观察方向
    vec3 viewDir = normalize(fs_in.ViewPos - fs_in.FragPos);
    
    // 初始化光照结果
    vec3 lighting = vec3(0.0);
    
    // 环境光照
    lighting += CalculateAmbientLight(albedo) * ao;
    
    // 方向光照
    if (directionalLightCount > 0) {
        lighting += CalculateDirectionalLight(directionalLight, normal, viewDir);
    }
    
    // 自发光
    vec3 emissive = material.emissiveColor * material.emissiveIntensity;
    if (material.useEmissiveMap) {
        emissive *= texture(emissiveMap, texCoord).rgb;
    }
    lighting += emissive;
    
    // 最终颜色（简单的色调映射）
    vec3 color = albedo * lighting;
    
    // Gamma校正
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, material.albedo.a);
}

vec3 CalculateNormal() {
    // 如果没有法线贴图，使用顶点法线
    if (!material.useNormalMap) {
        return normalize(fs_in.Normal);
    }
    
    // 从法线贴图采样
    vec3 normal = texture(normalMap, fs_in.TexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal.xy *= material.normalScale;
    normal = normalize(normal);
    
    // 构建TBN矩阵
    vec3 T = normalize(fs_in.Tangent);
    vec3 B = normalize(fs_in.Bitangent);
    vec3 N = normalize(fs_in.Normal);
    
    mat3 TBN = mat3(T, B, N);
    
    // 将法线从切线空间转换到世界空间
    return normalize(TBN * normal);
}

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    
    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.color * light.intensity * diff;
    
    // 镜面反射（简单的Blinn-Phong）
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = light.color * light.intensity * spec;
    
    // 简单的能量守恒：漫反射和镜面反射不能超过1.0
    return (diffuse + specular) * 0.5;
}

vec3 CalculateAmbientLight(vec3 albedo) {
    return ambientLight * albedo;
}