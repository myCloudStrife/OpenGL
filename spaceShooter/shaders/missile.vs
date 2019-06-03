#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 dir;

out VS_OUT {
    vec3 pos;
    vec3 dir;
} vs_out;

void main() {
    vs_out.pos = pos;
    vs_out.dir = normalize(dir);
}