#version 330 core

layout(points) in;
layout(points, max_vertices = 20) out;

in VS_OUT {
    vec4 color_type;
    vec4 pos_size;
    vec4 vel_lifetime;
} gs_in[];

out vec4 color_type;
out vec4 pos_size;
out vec4 vel_lifetime;

uniform float deltaTime;

uniform sampler2D texture0; //random

const float PARTICLE_SIMPLE = 0.0f;
const float PARTICLE_STARDUST = 1.0f;
const float PARTICLE_MISSILE = 2.0f;
const float PARTICLE_ASTEROID = 3.0f;


void main() {
    color_type = gs_in[0].color_type;
    if (color_type.w == PARTICLE_STARDUST) {
        vec3 position = gs_in[0].pos_size.xyz + gs_in[0].vel_lifetime.xyz * deltaTime;
        if (position.z > 60.0) {
            position.z = -500.0;
            position.xy = texture(texture0, position.xy).xy * 400.0 - 200.0;
        }
        pos_size = vec4(position, gs_in[0].pos_size.w);
        vel_lifetime = gs_in[0].vel_lifetime;
        EmitVertex();
    } else if (color_type.w == PARTICLE_ASTEROID) {
        for (int i = 0; i < 10; ++i) {
            vec3 r1 = texture(texture0, pos_size.xy * vel_lifetime.w  + i * deltaTime).xyz;
            color_type = vec4(vec3(r1.x), PARTICLE_SIMPLE);
            pos_size = gs_in[0].pos_size;
            pos_size.w = r1.y * 0.1;
            if (pos_size.z > -100.0) {
                pos_size.w += 0.05;
            } else {
                pos_size.w += 0.15;
            }
            vel_lifetime.w = 1.0 + r1.z * 2.0;
            vel_lifetime.xyz = (texture(texture0, r1.xy).xyz - 0.5) * 30.0;
            EmitVertex();
        }
        vel_lifetime = gs_in[0].vel_lifetime;
        vel_lifetime.w -= deltaTime;
        if (vel_lifetime.w > 0) {
            color_type = gs_in[0].color_type;
            pos_size = gs_in[0].pos_size;
            pos_size.xyz += vel_lifetime.xyz * deltaTime;
            EmitVertex();
        } else {
            vec3 r1 = texture(texture0, pos_size.xy).xyz;
            pos_size = vec4(r1.xy * 400.0 - 200.0, -500.0, r1.z * 0.4 + 0.15);
            color_type = vec4(vec3(0.9) + r1 * 0.1, PARTICLE_STARDUST);
            vel_lifetime = vec4(0.0, 0.0, 100.0, 0.0);
            EmitVertex();
        }
    } else if (color_type.w == PARTICLE_MISSILE) {
        for (int i = 0; i < 5; ++i) {
            vec3 r1 = texture(texture0, pos_size.xy * vel_lifetime.w + i * deltaTime).xyz;
            color_type.w = PARTICLE_SIMPLE;
            pos_size = gs_in[0].pos_size;
            pos_size.w = r1.y * 0.1;
            if (pos_size.z > -100.0) {
                pos_size.w += 0.1;
            } else if (pos_size.z > -200.0) {
                pos_size.w += 0.15;
            } else {
                pos_size.w += 0.2;
            }
            vel_lifetime = gs_in[0].vel_lifetime;
            vel_lifetime.w = 0.5 + r1.z;
            vel_lifetime.xyz += (texture(texture0, r1.xy).xyz - 0.5) * 20.0;
            EmitVertex();
        }
        vel_lifetime = gs_in[0].vel_lifetime;
        vel_lifetime.w -= deltaTime;
        if (vel_lifetime.w > 0) {
            color_type = gs_in[0].color_type;
            pos_size = gs_in[0].pos_size;
            pos_size.xyz += vel_lifetime.xyz * deltaTime;
            EmitVertex();
        } else {
            vec3 r1 = texture(texture0, pos_size.xy).xyz;
            pos_size = vec4(r1.xy * 400.0 - 200.0, -500.0, r1.z * 0.4 + 0.15);
            color_type = vec4(vec3(0.9) + r1 * 0.1, PARTICLE_STARDUST);
            vel_lifetime = vec4(0.0, 0.0, 100.0, 0.0);
            EmitVertex();
        }
    } else if (color_type.w == PARTICLE_SIMPLE) {
        color_type = gs_in[0].color_type;
        pos_size = gs_in[0].pos_size;
        vel_lifetime = gs_in[0].vel_lifetime;
        pos_size.xyz += vel_lifetime.xyz * deltaTime;
        vel_lifetime.w -= deltaTime;
        if (vel_lifetime.w > 0) {
            EmitVertex();
        }
    }
    EndPrimitive();
}