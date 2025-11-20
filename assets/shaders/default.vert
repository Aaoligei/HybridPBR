#version 460 core

// 顶点属性
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

// 输出到片段着色器
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

// 统一变量
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // 世界空间中的片段位置
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // 法线矩阵
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * aNormal);
    
    // 纹理坐标
    TexCoord = aTexCoord;
    
    // 最终顶点位置
    gl_Position = projection * view * vec4(FragPos, 1.0);
}