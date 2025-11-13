#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 Bitangent;
    vec3 ViewPos;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

void main() {
    // 计算世界空间位置
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.FragPos = worldPos.xyz;
    
    // 计算法线矩阵（用于法线变换）
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vs_out.Normal = normalize(normalMatrix * aNormal);
    
    // 传递纹理坐标
    vs_out.TexCoord = aTexCoord;
    
    // 传递切线和副切线
    vs_out.Tangent = normalize(normalMatrix * aTangent);
    vs_out.Bitangent = normalize(normalMatrix * aBitangent);
    
    // 传递观察位置
    vs_out.ViewPos = viewPos;
    
    // 计算裁剪空间位置
    gl_Position = projection * view * worldPos;
}