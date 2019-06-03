#version 330 core

in vec3 fragCoord;
in vec3 missileDir;
in vec3 center;

out vec4 fragColor;

uniform vec3 camPos;
uniform vec3 color;

float sdCapsule(vec3 p, vec3 a, vec3 b, float r) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h) - r;
}

void main() {
    vec3 rayDir = normalize(fragCoord - camPos);
    float sumd = 0;
    float mind = 100.0;
    for (int step = 0; step < 50; ++step) {
        float d = sdCapsule(camPos + rayDir * sumd - center, -2.0 * missileDir, 2.0 * missileDir, 0.5);
        sumd += d;
        mind = min(mind, d);
        if (d < 0.001) {
            fragColor = vec4(1.0, 1.0, 1.0, 1.0) - vec4(vec3(1.0) - color, 0.0) * (step - 1) / 50.0;
            return;
        }
    }
    fragColor = vec4(color, 1.0) * 0.05 / mind;
}