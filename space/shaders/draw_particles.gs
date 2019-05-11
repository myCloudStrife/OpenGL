#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec4 color_type;
    vec4 pos_size;
} gs_in[];

out vec4 color_type;
out vec2 coords;

uniform mat4 VP;
uniform vec3 camPos;

void main() {
    color_type = gs_in[0].color_type;
    vec3 pos = gs_in[0].pos_size.xyz;
    float size = gs_in[0].pos_size.w;
    vec3 viewDir = normalize(pos - camPos);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(viewDir, up);
    gl_Position = VP * vec4(pos + (up - right) * size, 1.0);  //left upper
    coords = vec2(-1.0, 1.0);
    EmitVertex();
    gl_Position = VP * vec4(pos - (up + right) * size, 1.0);  //left bottom
    coords = vec2(-1.0, -1.0);
    EmitVertex();
    gl_Position = VP * vec4(pos + (up + right) * size, 1.0);  //right upper
    coords = vec2(1.0, 1.0);
    EmitVertex();
    gl_Position = VP * vec4(pos - (up - right) * size, 1.0);  //right bottom
    coords = vec2(1.0, -1.0);
    EmitVertex();
    EndPrimitive();
}