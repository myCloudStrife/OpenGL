#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec3 pos;
} gs_in[];

out vec2 fragmentTexCoord;

uniform mat4 VP;
uniform vec3 camPos;
uniform int timer;

void main() {
    vec3 pos = gs_in[0].pos;
    float size = 20.0;
    vec3 viewDir = normalize(pos - camPos);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(viewDir, up);
    int t = 60 - timer;
    vec2 corner = vec2((t % 8) / 8.0, (t / 8) / 8.0);
    //vec2 corner = vec2(7.0 / 8.0);
    gl_Position = VP * vec4(pos + (up - right) * size, 1.0);  //left upper
    fragmentTexCoord = corner + vec2(0.0, 1.0 / 8.0);
    EmitVertex();
    gl_Position = VP * vec4(pos - (up + right) * size, 1.0);  //left bottom
    fragmentTexCoord = corner + vec2(0.0, 0.0);
    EmitVertex();
    gl_Position = VP * vec4(pos + (up + right) * size, 1.0);  //right upper
    fragmentTexCoord = corner + vec2(1.0 / 8.0, 1.0 / 8.0);
    EmitVertex();
    gl_Position = VP * vec4(pos - (up - right) * size, 1.0);  //right bottom
    fragmentTexCoord = corner + vec2(1.0 / 8.0, 0.0);
    EmitVertex();
    EndPrimitive();
}