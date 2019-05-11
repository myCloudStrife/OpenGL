#version 330 core

in vec3 FragCoord;
in vec2 TexCoord;
in mat3 TBN;

out vec4 FragColor;

uniform sampler2D texture0; //ambient occlusion
uniform sampler2D texture1; //diffuse color
uniform sampler2D texture2; //specular color
uniform sampler2D texture3; //normal map

uniform vec3 camPos;
uniform int lightType;

struct DirectionLight {
    vec3 direction;
    vec3 colorIntensity;
};

struct PointLight {
    vec3 position;
    vec3 colorIntensity;
};

uniform PointLight pl;
//uniform DirectionLight sun;

DirectionLight sun = DirectionLight(vec3(0, -1, 0), vec3(0.7));

void main() {
    
    vec3 lightDir;
    vec3 colorIntensity;
    float dist2;
    if (lightType == 1) {
        lightDir = normalize(pl.position - FragCoord);
        dist2 = distance(FragCoord, pl.position);
        dist2 = dist2 * dist2;
        colorIntensity = pl.colorIntensity;
    } else {
        lightDir = -sun.direction;
        dist2 = 1.0;
        colorIntensity = sun.colorIntensity;
    }
    
    vec3 eyeDir = normalize(camPos - FragCoord);
    vec3 AO, diffuse, spec, n; 
    
    AO = texture(texture0, TexCoord).xyz;
    diffuse = texture(texture1, TexCoord).xyz;
    spec = texture(texture2, TexCoord).xyz;
    n = texture(texture3, TexCoord).xyz * 2 - 1;
    n = normalize(TBN * n);

    vec3 color = vec3(0);
    
    float LdotN = max(dot(lightDir, n), 0);
    color += colorIntensity * diffuse * LdotN / dist2;
    if (LdotN > 0) {
        vec3 h = normalize(lightDir + eyeDir); //Halfway(Blinn-Phong)
        color += colorIntensity * LdotN * spec * pow(max(dot(h, n), 0), 77) / dist2;
    }
    
    color += 0.2 * diffuse * AO;
    FragColor = vec4(color, 1);
    //FragColor = vec4(n, 1);
    //FragColor = vec4(1, 0, 0, 1);
}