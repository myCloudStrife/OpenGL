#version 330 core

layout(location = 0) in vec3 vertex;

uniform mat4 model;
uniform mat4 ViewProj;

void main(void) {
    gl_Position = ViewProj * model * vec4(vertex, 1.0f);
}
