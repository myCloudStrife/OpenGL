#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 color;
uniform sampler2D text;

void main() {
    FragColor = texture(text, TexCoord).x * vec4(color, 1.0);
}