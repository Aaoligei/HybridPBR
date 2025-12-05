#version 460 core

// 输出对应 GBuffer::CreateFramebuffer 中的 Color Attachments 0-4
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out vec4 gMRA;      // R=Metallic, G=Roughness, B=AO
layout (location = 4) out vec3 gEmissive;

in VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

// 材质属性 Uniforms (对应 PBRMaterial::ApplyToShader)
// 假设 C++ 设置了以下 Uniform，如果没贴图应绑定白色/默认纹理
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D emissiveMap;

// 开关标记 (可选，取决于你的材质系统实现)
uniform bool useNormalMap;

void main() {
    // 1. Position: 写入世界空间坐标 (RGB16F)
    gPosition = fs_in.WorldPos;

    // 2. Normal: 处理法线贴图 (RGB16F)
    vec3 N = normalize(fs_in.Normal);
    if (useNormalMap) {
        vec3 normalMapValue = texture(normalMap, fs_in.TexCoords).rgb;
        normalMapValue = normalMapValue * 2.0 - 1.0;
        N = normalize(fs_in.TBN * normalMapValue);
    }
    gNormal = N;

    // 3. Albedo (RGBA8)
    // 这里的 Alpha 通道可以存储透明度测试结果，但在延迟渲染中通常只写 1.0
    // 或者用于标记材质 ID
    gAlbedo = texture(albedoMap, fs_in.TexCoords) ;

    // 4. MRA: Metallic, Roughness, AO (RGBA8)
    // 注意：很多 PBR 流程将 M/R/AO 打包在一张贴图里，这里假设是分开的或通道提取的
    float metallic = texture(metallicMap, fs_in.TexCoords).r ;
    float roughness = texture(roughnessMap, fs_in.TexCoords).r ;
    float ao = texture(aoMap, fs_in.TexCoords).r ;
    
    gMRA = vec4(metallic, roughness, ao, 1.0);

    // 5. Emissive (RGB16F)
    gEmissive = texture(emissiveMap, fs_in.TexCoords).rgb ;
}