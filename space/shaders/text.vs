#version 330 core

layout(location = 0) in vec3 edges;
layout(location = 1) in vec2 uv;

out vec2 TexCoord;

uniform float offset;
uniform float left;
uniform float down;
uniform float up;

void main(void)
{
    TexCoord = vec2(uv.x + 1.0 / 13 * edges.x, uv.y + 76.0 / 512 * (1 - edges.y));
    vec2 coord = vec2(left + offset * (edges.x + gl_InstanceID), down + (up - down) * edges.y);
    gl_Position = vec4(coord, -1.0f, 1.0f);
}
