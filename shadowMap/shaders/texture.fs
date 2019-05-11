#version 330 core

in vec2 fragmentTexCoord;

out vec4 fragColor;

uniform sampler2D texture0;

uniform float near = 0.1;
uniform float far = 50;
uniform int lightType;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main(void) {
    float depth = texture(texture0, fragmentTexCoord).r;
    if (lightType == 1) {
        fragColor = vec4(vec3(LinearizeDepth(depth) / far), 1);
    } else {
        fragColor = vec4(vec3(depth), 1);
    }
}