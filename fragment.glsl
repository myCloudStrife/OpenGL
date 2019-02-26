#version 330

#define float2 vec2
#define float3 vec3
#define float4 vec4
#define float4x4 mat4
#define float3x3 mat3

const float EPSILON = 0.0001;
const float MAX_DISTANCE = 50.0;
const int MAX_STEP = 1024;
const int LIGHTS_NUM = 2;

struct Light {
    vec3 pos;
    vec3 intensity;
};

in vec2 fragmentTexCoord;

layout(location = 0) out vec4 fragColor;

uniform int g_screenWidth;
uniform int g_screenHeight;

uniform mat4 g_rotate;
uniform vec3 g_position;

vec3 EyeRayDir(vec2 coord, vec2 size) {
    float fov = 3.141592654f / 2.0f; //90 degrees
    vec3 ray_dir = vec3(coord - size / 2.0f, -size.x / tan(fov / 2.0f));
    return normalize(ray_dir);
}

float distSphere(vec3 p, float r) {
    return length(p) - r;
}

float distRoundBox(vec3 p, vec3 b, float r) {
  return length(max(abs(p) - b, 0)) - r;
}

float distHexPrism(vec3 p, vec2 h) {
    vec3 q = abs(p);
    return max(q.z - h.y, max((q.x*0.866025 + q.y*0.5), q.y) - h.x);
}

float distHexPrism2(vec3 p, vec2 h) {
    vec3 q = abs(mod(p, vec3(4, 4, 0)) - vec3(2, 2, 0));
    return max(q.z - h.y, max((q.x*0.866025 + q.y*0.5), q.y) - h.x);
}

float distTerrain(vec3 p) {
    return p.y + 2;
}

float distScene(vec3 p) {
    float dmin = 1e38;
    dmin = min(dmin, distSphere(p - vec3(0, 0, -1), 1));
    dmin = min(dmin, distRoundBox(p - vec3(5, 0, 0), vec3(1, 1, 2), 0.05));
    dmin = min(dmin, distHexPrism2((p - vec3(0, -2, 1)).xzy, vec2(1, 0.5)));
    dmin = min(dmin, distHexPrism2((p - vec3(0, -1.7, -1)).xzy, vec2(1, 0.5)));
    dmin = min(dmin, distHexPrism2((p - vec3(2, -1.85, 0)).xzy, vec2(1, 0.5)));
    dmin = min(dmin, distHexPrism2((p - vec3(2, -1.5, 2)).xzy, vec2(1, 0.5)));
    dmin = min(dmin, distTerrain(p));
    return dmin;
}


float getShortDistance(vec3 src, vec3 dir) {
    float dist, fulldist = 0;
    int step;
    for (step = 0; step < MAX_STEP; step++) {
        dist = distScene(src + dir * fulldist);
        fulldist += dist;
        if (dist < EPSILON || fulldist > MAX_DISTANCE) {
            break;
        }
    }
    if (step == MAX_STEP) {
        return MAX_DISTANCE;
    }
    return fulldist;
}

vec3 estimateNormal(float3 z) {
    vec3 z1 = z + vec3(EPSILON, 0, 0);
    vec3 z2 = z - vec3(EPSILON, 0, 0);
    vec3 z3 = z + vec3(0, EPSILON, 0);
    vec3 z4 = z - vec3(0, EPSILON, 0);
    vec3 z5 = z + vec3(0, 0, EPSILON);
    vec3 z6 = z - vec3(0, 0, EPSILON);
    float dx = distScene(z1) - distScene(z2);
    float dy = distScene(z3) - distScene(z4);
    float dz = distScene(z5) - distScene(z6);
    return normalize(vec3(dx, dy, dz) / (2.0 * EPSILON));
}

vec3 illumPhong(vec3 kd, vec3 ks, float shininess, vec3 p,
        vec3 light_pos, vec3 intensity, vec3 eye_dir) {
    vec3 n = estimateNormal(p);
    vec3 l = normalize(light_pos - p);
    vec3 r = normalize(reflect(-l, n));
    vec3 color = kd * max(dot(l, n), 0) * intensity;
    color += ks * pow(max(dot(r, -eye_dir), 0.0), shininess) * intensity;
    return color;
}

vec3 illumination(vec3 p, vec3 eye_dir) {
    
    Light lights[LIGHTS_NUM];
    lights[0].pos = vec3(2, 2, 1);
    lights[0].intensity = vec3(1, 1, 1);
    lights[1].pos = vec3(-4, 1, 1);
    lights[1].intensity = vec3(1, 1, 1);
    
    mat3 k;
    if (distTerrain(p) < EPSILON * 2) {
        k = mat3(
                0, 0.7, 0,
                0.1, 0.35, 0.1,
                0.45, 0.55, 0.45
        );
    } else {
        k = mat3(
                0, 0.05, 0.05,
                0.4, 0.5, 0.5,
                0.04, 0.7, 0.7
        );
    }
    vec3 color = k[0] * vec3(0.5, 0.5, 0.5);
    for (int i = 0; i < LIGHTS_NUM; ++i) {
        float d = getShortDistance(lights[i].pos, normalize(p - lights[i].pos));
        if (d > length(p - lights[i].pos) - EPSILON * 10) {
            color += illumPhong(k[1], k[2], 25, p, lights[i].pos, lights[i].intensity, eye_dir);
        }
    }
    return color;
}


void main(void) {

    vec2 screenSize = vec2(g_screenWidth, g_screenHeight);
    vec2 coord = fragmentTexCoord * screenSize;
    
    vec3 ray_pos = g_position; 
    vec3 ray_dir = EyeRayDir(coord, screenSize);
    ray_dir = float3x3(g_rotate) * ray_dir;
    
    float dist = getShortDistance(ray_pos, ray_dir);
    
    
    
    if (dist < MAX_DISTANCE - EPSILON) {
        ray_pos += ray_dir * dist;
        fragColor = vec4(illumination(ray_pos, ray_dir), 1);
        //dist = length(ray_pos - vec3(5, 5, 0));
        /*if (distTerrain(ray_pos) < EPSILON * 2) {
            //fragColor = vec4(illumPhong(vec3(0,0,0), vec3(0.1,0.35,0.1), vec3(0.45,0.55,0.45),
            //        25, ray_pos, vec3(5, 5, 0), vec3(0.8, 0.8, 0.8), ray_dir), 1);
            fragColor = vec4(illumination(ray_pos, ray_dir), 1);
            //fragColor = vec4(vec3(0.2, 0.7, 0.3) * (0.5 + 1 / dist), 1);
        } else {
            fragColor = vec4(illumPhong(vec3(0,0.05,0.05), vec3(0.4,0.5,0.5), vec3(0.04,0.7,0.7),
                    7.8, ray_pos, vec3(5, 5, 0), vec3(0.8, 0.8, 0.8), ray_dir), 1);
            //fragColor = vec4(1,0,0,1);
            //fragColor = vec4(vec3(0.5, 0.5, 0.75) / (dist * dist), 1);
        }*/
    } else {
        fragColor = vec4(0, 0, 0, 1);
    }
    
    /*float tmin = 1e38f;
    float tmax = 0;

    if(!RayBoxIntersection(ray_pos, ray_dir, g_bBoxMin, g_bBoxMax, tmin, tmax))
    {
        fragColor = g_bgColor;
        return;
    }
	
	float alpha = 1.0f;
	float3 color = RayMarchConstantFog(tmin, tmax, alpha);
	fragColor = float4(color,0)*(1.0f-alpha) + g_bgColor*alpha;*/

}


