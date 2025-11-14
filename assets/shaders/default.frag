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

// PBR工具函数
vec3 CalculateNormal();
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
bool IsTextureBound(sampler2D tex);

void main() {
    // 应用纹理缩放和偏移
    vec2 texCoord = fs_in.TexCoord * material.textureScale + material.textureOffset;
    
    // 确保纹理坐标在[0,1]范围内（可选，取决于想要的重复效果）
    // texCoord = fract(texCoord);
    
    // 获取基础颜色
    vec3 albedo = material.albedo.rgb;
    if (material.useAlbedoMap && IsTextureBound(albedoMap)) {
        albedo = texture(albedoMap, texCoord).rgb;
    }
    
    // 获取法线
    vec3 normal = CalculateNormal();
    
    // 获取其他材质参数
    float metallic = material.metallic;
    float roughness = material.roughness;
    float ao = material.ambientOcclusion;
    
    // 只有在启用了金属贴图且纹理已绑定时才使用metallic贴图
    if (material.useMetallicMap && IsTextureBound(metallicMap)) {
        metallic = texture(metallicMap, texCoord).r;
    }
    
    // 只有在启用了粗糙贴图且纹理已绑定时才使用roughness贴图
    if (material.useRoughnessMap && IsTextureBound(roughnessMap)) {
        roughness = texture(roughnessMap, texCoord).r;
    }
    
    if (material.useAOMap && IsTextureBound(aoMap)) {
        ao = texture(aoMap, texCoord).r;
    }
    
    // 确保metallic和roughness在有效范围内
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.05, 1.0); // 防止除零错误
    
    // 计算观察方向
    vec3 viewDir = normalize(fs_in.ViewPos - fs_in.FragPos);
    
    // 初始化光照结果
    vec3 lighting = vec3(0.0);
    
    // 环境光照（简化版）
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    vec3 kS = fresnelSchlick(max(dot(normal, viewDir), 0.0), F0);
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;  
    
    vec3 irradiance = ambientLight;
    vec3 diffuse    = irradiance * albedo;
    lighting += (kD * diffuse) * ao;
    
    // 方向光照
    if (directionalLightCount > 0) {
        vec3 lightDir = normalize(-directionalLight.direction);
        vec3 radiance = directionalLight.color * directionalLight.intensity;
        
        // Cook-Torrance BRDF
        vec3 H = normalize(viewDir + lightDir);
        float NDF = DistributionGGX(normal, H, roughness);        
        float G   = GeometrySmith(normal, viewDir, lightDir, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);       
        
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;
        
        // 能量守恒
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	
        
        // 缩放光线与表面法线的点积
        float NdotL = max(dot(normal, lightDir), 0.0);        
        lighting += (kD * albedo / 3.14159265359 + specular) * radiance * NdotL;
    }
    
    // 自发光
    vec3 emissive = material.emissiveColor * material.emissiveIntensity;
    if (material.useEmissiveMap && IsTextureBound(emissiveMap)) {
        emissive *= texture(emissiveMap, texCoord).rgb;
    }
    lighting += emissive;
    
    // Gamma校正
    vec3 color = lighting;
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, material.albedo.a);
}

vec3 CalculateNormal() {
    // 如果没有法线贴图，使用顶点法线
    if (!material.useNormalMap) {
        return normalize(fs_in.Normal);
    }
    
    // 检查法线贴图是否存在
    if (!IsTextureBound(normalMap)) {
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

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359 * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 检查纹理是否已绑定
bool IsTextureBound(sampler2D tex) {
    return textureSize(tex, 0).x > 0;
}