
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec3 ViewPos;
    vec3 WorldPos;
} vs_out;

uniform mat4 model;
uniform mat3 normalMatrix;

layout (std140, binding = 0) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec3 viewPos;
};

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.FragPos = worldPos.xyz;
    vs_out.WorldPos = worldPos.xyz;
    
    // 法线矩阵
    vs_out.Normal = normalize(normalMatrix * aNormal);
    
    vs_out.TexCoord = aTexCoord;
    vs_out.ViewPos = viewPos;
    
    gl_Position = projection * view * worldPos;
}

