#version 330 core

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 UV;

layout(location = 6) in vec4 pos_scale;
layout(location = 7) in vec4 rotax_angle;

out vec3 FragCoord;
out vec2 TexCoord;
out mat3 TBN;

uniform int source;

uniform vec4 uni_pos_scale;
uniform vec4 uni_rotax_angle;

uniform mat4 uni_model;
uniform mat4 VP;

mat4 createModelMatrix(vec4 pos_scale, vec4 rotax_angle) {
    vec3 ax = rotax_angle.xyz;
    float s = sin(rotax_angle.w);
    float c = cos(rotax_angle.w);
    float oc = 1.0 - c;
    float sc = pos_scale.w;
    return mat4((oc * ax.x * ax.x + c) * sc,        (oc * ax.x * ax.y - ax.z * s) * sc, (oc * ax.z * ax.x + ax.y * s) * sc, 0.0,
                (oc * ax.x * ax.y + ax.z * s) * sc, (oc * ax.y * ax.y + c) * sc,        (oc * ax.y * ax.z - ax.x * s) * sc, 0.0,
                (oc * ax.z * ax.x - ax.y * s) * sc, (oc * ax.y * ax.z + ax.x * s) * sc, (oc * ax.z * ax.z + c) * sc,        0.0,
                pos_scale.x,                        pos_scale.y,                        pos_scale.z,                        1.0);
}

void main() {
    TexCoord = UV;

    mat4 model;
    if (source == 1) {
        model = createModelMatrix(pos_scale, rotax_angle);
    } else if (source == 2) {
        model = uni_model;
    } else {
        model = createModelMatrix(uni_pos_scale, uni_rotax_angle);
    }

    vec3 T = normalize(mat3(model) * normalize(tangent));
    vec3 N = normalize(mat3(model) * normalize(normal));
    vec3 B = normalize(mat3(model) * normalize(bitangent));
    TBN = mat3(T, B, N);
    
    FragCoord = (model * vec4(vertex, 1.0f)).xyz;
    gl_Position = VP * model * vec4(vertex, 1.0f);
}
