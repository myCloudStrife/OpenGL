#version 330 core

in vec2 fragmentTexCoord;

out vec4 fragColor;

uniform vec3 color;

uniform sampler2D texture0;

void main(void) {
    fragColor = texture(texture0, fragmentTexCoord);
    fragColor.xyz *= color;
}