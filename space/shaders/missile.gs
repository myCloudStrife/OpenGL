#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec3 pos;
    vec3 dir;
} gs_in[];

out vec3 fragCoord;
out vec3 missileDir;
out vec3 center;

uniform mat4 VP;
uniform vec3 camPos;

void main() {
    vec3 pos = gs_in[0].pos;
    float size = 2.0;
    vec3 viewDir = normalize(pos - camPos);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(viewDir, up);
    missileDir = gs_in[0].dir;
    center = pos;
    fragCoord = pos + (up - right) * size;
    gl_Position = VP * vec4(fragCoord, 1.0);  //left upper
    EmitVertex();
    fragCoord = pos - (up + right) * size;
    gl_Position = VP * vec4(fragCoord, 1.0);  //left bottom
    EmitVertex();
    fragCoord = pos + (up + right) * size;
    gl_Position = VP * vec4(fragCoord, 1.0);  //right upper
    EmitVertex();
    fragCoord = pos - (up - right) * size;
    gl_Position = VP * vec4(fragCoord, 1.0);  //right bottom
    EmitVertex();
    EndPrimitive();
}