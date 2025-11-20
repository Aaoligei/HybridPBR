#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

uniform vec3 color;
uniform sampler2D albedoMap;
uniform bool useTexture;

void main() {
    if (useTexture) {
        FragColor = texture(albedoMap, TexCoord);
    } else {
        FragColor = vec4(color, 1.0);
    }
}