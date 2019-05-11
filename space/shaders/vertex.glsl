#version 330 core

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 UV;

out vec3 FragCoord;
out vec2 TexCoord;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(void) {
    TexCoord = UV;

    vec3 T = normalize(mat3(model) * normalize(tangent));
    vec3 N = normalize(mat3(model) * normalize(normal));
    vec3 B = normalize(mat3(model) * normalize(bitangent));
    TBN = mat3(T, B, N);
    
    FragCoord = (model * vec4(vertex, 1.0f)).xyz;
    gl_Position = projection * view * model * vec4(vertex, 1.0f);
}
