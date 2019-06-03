#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 FragCoord;
    vec2 TexCoord;
    mat3 TBN;
} gs_in[];

out vec3 FragCoord;
out vec2 TexCoord;
out mat3 TBN;

uniform float time;

vec4 explode(vec4 position, vec3 normal) {
    float vel = 10.0;
    return position + vec4(normal * time * vel, 0.0);
}

vec3 GetNormal()
{
   vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
   vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
   return normalize(cross(a, b));
}

void main() {    
    vec3 normal = GetNormal();
    vec4 position;
    
    position = mix(gl_in[0].gl_Position, (gl_in[1].gl_Position + gl_in[2].gl_Position) * 0.5, time * 0.5);
    gl_Position = explode(position, normal);
    FragCoord = gs_in[0].FragCoord;
    TexCoord = gs_in[0].TexCoord;
    TBN = gs_in[0].TBN;
    EmitVertex();
    position = mix(gl_in[1].gl_Position, (gl_in[0].gl_Position + gl_in[2].gl_Position) * 0.5, time * 0.5);
    gl_Position = explode(position, normal);
    //gl_Position = gl_in[1].gl_Position;
    //gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
    FragCoord = gs_in[1].FragCoord;
    TexCoord = gs_in[1].TexCoord;
    TBN = gs_in[1].TBN;
    EmitVertex();
    position = mix(gl_in[2].gl_Position, (gl_in[0].gl_Position + gl_in[1].gl_Position) * 0.5, time * 0.5);
    gl_Position = explode(position, normal);
    //gl_Position = gl_in[2].gl_Position;
    //gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
    FragCoord = gs_in[2].FragCoord;
    TexCoord = gs_in[2].TexCoord;
    TBN = gs_in[2].TBN;
    EmitVertex();
    EndPrimitive();
}