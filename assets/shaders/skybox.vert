#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 localPos;

void main()
{
    localPos = aPos;
    vec4 clipPos = projection * view * vec4(localPos, 1.0);

    gl_Position = clipPos.xyww;
}