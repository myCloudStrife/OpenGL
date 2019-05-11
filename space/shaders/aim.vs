#version 330 core

out VS_OUT {
    vec2 coord;
} vs_out;

uniform vec2 coord;

void main() {
    vs_out.coord = coord;
}