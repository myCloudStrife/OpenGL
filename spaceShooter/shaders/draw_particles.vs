#version 330 core

layout(location = 0) in vec4 color_type;
layout(location = 1) in vec4 pos_size;

out VS_OUT {
    vec4 color_type;
    vec4 pos_size;
} vs_out;

void main() {
    vs_out.color_type = color_type;
    vs_out.pos_size = pos_size;
}