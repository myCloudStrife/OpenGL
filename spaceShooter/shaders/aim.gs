#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec2 coord;
} gs_in[];

out vec2 fragmentTexCoord;

const float aspect = 4.0 / 3.0;

void main() {
    vec2 coord = gs_in[0].coord;
    float size = 0.1;
    fragmentTexCoord = vec2(0.0, 0.0);
    gl_Position = vec4(coord + vec2(-1.0, aspect) * size, 0.0, 1.0);  //left upper
    EmitVertex();
    fragmentTexCoord = vec2(0.0, 1.0);
    gl_Position = vec4(coord + vec2(-1.0, -aspect) * size, 0.0, 1.0);  //left bottom
    EmitVertex();
    fragmentTexCoord = vec2(1.0, 0.0);
    gl_Position = vec4(coord + vec2(1.0, aspect) * size, 0.0, 1.0);  //right upper
    EmitVertex();
    fragmentTexCoord = vec2(1.0, 1.0);
    gl_Position = vec4(coord + vec2(1.0, -aspect) * size, 0.0, 1.0);  //right bottom
    EmitVertex();
    EndPrimitive();
}