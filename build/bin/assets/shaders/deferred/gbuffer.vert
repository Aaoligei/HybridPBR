#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
} vs_out;

// Binding 0: Camera Data (对应 C++ CameraUBO)
layout (std140, binding = 0) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec3 viewPos;
};

uniform mat4 model;
uniform mat3 normalMatrix;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.WorldPos = worldPos.xyz;
    vs_out.TexCoords = aTexCoords;
    
    // 计算法线矩阵 (World Space)
    vs_out.Normal = normalize(normalMatrix * aNormal);
    
    // 计算 TBN 矩阵用于法线贴图
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt 正交化
    vec3 B = cross(N, T);
    vs_out.TBN = mat3(T, B, N);

    gl_Position = projection * view * worldPos;
}