#version 330 core

in vec4 color_type;
in vec2 coords;

out vec4 FragColor;

uniform sampler2D texture0; //circle

void main() {
    vec2 texCoord = (coords + 1.0) * 0.5;
    FragColor = texture(texture0, texCoord) * vec4(color_type.xyz, 1.0);
    /*float brightness = max(1.0 - length(coords), 0);
    brightness = min(brightness * 1.5, 1.0);
    FragColor = vec4(color_type.xyz, 1.0) * brightness;*/
}