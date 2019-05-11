#version 330 core

out VS_OUT {
    vec3 pos;
} vs_out;

uniform vec3 pos;

void main() {
    vs_out.pos = pos;
}