#version 460 core
// [核心] 开启 Bindless 纹理扩展
#extension GL_ARB_bindless_texture : require

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec3 ViewPos;
    vec3 WorldPos;
} fs_in;

layout (std140, binding = 2) uniform MaterialData {
    // 0-16
    vec4 material_albedoFactor;      
    // 16-32
    vec3 material_emissiveFactor;    
    float material_emissiveIntensity;
    
    // 32-48
    float material_metallicFactor;   
    float material_roughnessFactor;  
    float material_aoFactor;         
    float material_normalScale;      
    
    // 48-96 (Bindless Handles, sampler2D = 8 bytes)
    sampler2D material_albedoMap;    
    sampler2D material_normalMap;    
    sampler2D material_metallicMap;  
    sampler2D material_roughnessMap; 
    sampler2D material_aoMap;        
    sampler2D material_emissiveMap;  
    
    // 96-120 (Flags, int = 4 bytes)
    int material_useAlbedoMap;
    int material_useNormalMap;
    int material_useMetallicMap;
    int material_useRoughnessMap;
    int material_useAOMap;
    int material_useEmissiveMap;
    
    // 120-128 (Padding)
    float pad3;
    float pad4;
};

// --- 光照数据 (保持不变) ---
struct Light {
    vec3 position;  float pad0;
    vec3 direction; float pad1;
    vec3 color;     float intensity;
    float range;    float constant;
    float linear;   float quadratic;
    float innerCutoff; float outerCutoff;
    int type;       float pad2, pad3, pad4;
};

layout (std140, binding = 1) uniform LightData {
    int lightCount;
    int pad0, pad1, pad2;
    Light lights[16];
};

const float PI = 3.14159265359;

// --- PBR 函数 (保持不变) ---
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 0.0001);
}
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

void main()
{       
    // 1. 读取材质属性 (从 UBO)
    vec3 albedo = material_albedoFactor.rgb;
    // 使用 Bindless 纹理采样
    if (material_useAlbedoMap != 0) {
        albedo = texture(material_albedoMap, fs_in.TexCoord).rgb;
        albedo = pow(albedo, vec3(2.2)); // sRGB -> Linear
    }
    

    float roughness = material_roughnessFactor;
    if (material_useRoughnessMap != 0) {
        // 假设 roughness 在 G 通道
        roughness = texture(material_roughnessMap, fs_in.TexCoord).g;
    }

    float metallic = material_metallicFactor;
    if (material_useMetallicMap != 0) {
        // 假设 metallic 在 B 通道
        metallic = texture(material_metallicMap, fs_in.TexCoord).b;
    }
    
    // 简单的法线处理 (暂不处理法线贴图，先把颜色跑通)
    vec3 N = normalize(fs_in.Normal);
    vec3 V = normalize(fs_in.ViewPos - fs_in.WorldPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // 2. 光照计算
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < lightCount; ++i) {
        vec3 L = normalize(lights[i].position - fs_in.WorldPos);
        vec3 H = normalize(V + L);
        
        float distance = length(lights[i].position - fs_in.WorldPos);
        float attenuation = 1.0 / (distance * distance); // 简单物理衰减
        if (lights[i].type == 0) { // Directional
             L = normalize(-lights[i].direction);
             attenuation = 1.0;
        }
        
        vec3 radiance = lights[i].color * lights[i].intensity * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  

        float NdotL = max(dot(N, L), 0.0);        
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }   
    
    // 3. 环境光 + 自发光
    vec3 ambient = vec3(0.03) * albedo * material_aoFactor;
    vec3 color = ambient + Lo;

    // Tone mapping
    color = color / (color + vec3(1.0));
    // Gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}