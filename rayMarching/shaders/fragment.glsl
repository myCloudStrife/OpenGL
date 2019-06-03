#version 330

const float EPSILON = 0.0001;
const float MAX_DISTANCE = 60.0;
const int MAX_STEP = 1024;
const int MAX_REFLECTION = 4;
const int MAX_REFRACTION = 4;
const int LIGHTS_NUM = 2;

in vec2 fragmentTexCoord;

layout(location = 0) out vec4 fragColor;

uniform int g_litemode;

uniform int g_screenWidth;
uniform int g_screenHeight;

uniform float g_time;

uniform mat4 g_rotate;
uniform vec3 g_position;

uniform samplerCube skybox;

struct Light {
    vec3 pos;
    vec3 intensity;
};

Light lights[2] = Light[2](
        Light(vec3(0, 4, 3), vec3(1, 1, 1)),
        Light(vec3(0, 2, 0), vec3(1, 1, 1)));

int FRACTAL_ITERATIONS = 4;

vec3 EyeRayDir(vec2 coord, vec2 size) {
    float fov = 3.141592654f / 2.0f; //90 degrees
    vec3 ray_dir = vec3(coord - size / 2.0f, -size.x / tan(fov / 2.0f));
    return normalize(ray_dir);
}

vec3 rotateX(vec3 p, float phi) {
    return vec3(p.x, p.y * cos(phi) - p.z * sin(phi), p.z * cos(phi) + p.y * sin(phi));
}

vec3 rotateZ(vec3 p, float phi) {
    return vec3(p.x * cos(phi) - p.y * sin(phi), p.y * cos(phi) + p.x * sin(phi), p.z);;
}

float length8(vec2 p) {
    vec2 pp = p * p;
    vec2 p4 = pp * pp;
    return pow(dot(p4, p4), 0.125);
}

vec2 fractalSphere(vec3 p, float r) {
    vec2 dist_lvl = vec2(1e38, 0.0);
    for (int i = 0; i < FRACTAL_ITERATIONS; i++)
    {
        p = rotateX(p, g_time / 4);
        float d = length(p) - r;
        vec3 hole = abs(p) - r * vec3(0.4082);
        d = max(d, -(length(hole) - r * 0.5));
        if (dist_lvl.x > d) {
            dist_lvl = vec2(d, i);
        }
        p = vec3(hole.z, hole.y, -hole.x);
        r *= 0.333;
    }
    float d = length(p) - r;
    if (dist_lvl.x > d) {
        dist_lvl = vec2(d, FRACTAL_ITERATIONS);
    }
    return dist_lvl;
}

float distSphere(vec3 p, float r) {
    return length(p) - r;
}

float distRoundBox(vec3 p, vec3 b, float r) {
    return length(max(abs(p) - b, 0)) - r;
}

float distTorus82(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length8(q) - t.y;
}

float distHexPrismRep(vec3 p, vec2 h) {
    vec3 q = abs(mod(p, vec3(4, 4, 0)) - vec3(2, 2, 0));
    return max(q.z - h.y, max((q.x*0.866025 + q.y*0.5), q.y) - h.x);
}

float distTerrain(vec3 p) {
    if (g_litemode > 0) {
        return distRoundBox(p - vec3(0, -1.25, 0), vec3(4, 0.25, 4), 0.05);
    }
    float dmin = distHexPrismRep((p - vec3(0, -4, 1)).xzy, vec2(1, 2));
    dmin = min(dmin, distHexPrismRep((p - vec3(0, -3.7, -1)).xzy, vec2(1, 2)));
    dmin = min(dmin, distHexPrismRep((p - vec3(2, -3.85, 0)).xzy, vec2(1, 2)));
    dmin = min(dmin, distHexPrismRep((p - vec3(2, -3.5, 2)).xzy, vec2(1, 2)));
    return dmin;
}

float distMirror(vec3 p) {
    return distSphere(p - vec3(0, 2, 0), 1.25);
}

float distLens(vec3 p) {
    return abs(max(distSphere(p - vec3(0, 9, 5), 5.5),
            distRoundBox(p - vec3(0, 9, -1), vec3(3, 3, 1), 0.01)));
}

float distRings(vec3 p) {
    float dmin = distTorus82((p - vec3(0, 9, 0)).xzy, vec2(2.45, 0.15));
    dmin = min(dmin, distTorus82(rotateX(p - vec3(0, 2, 0), -0.5), vec2(3, 0.15)));
    return min(dmin, distTorus82(rotateX(p - vec3(0, 2, 0), -1), vec2(3, 0.15)));
}

float distPillar(vec3 p) {
    p.x = abs(p.x);
    return distRoundBox(rotateZ(p - vec3(2.87, 3.73, 0), -0.08), vec3(0.1, 5.25, 0.1), 0.04);
}

float distScene(vec3 p) {
    float dmin = 1e38;
    dmin = min(dmin, distRings(p));
    dmin = min(dmin, distMirror(p));
    dmin = min(dmin, distLens(p));
    dmin = min(dmin, fractalSphere(p - vec3(0, 9, 20), 5).x);
    dmin = min(dmin, distPillar(p));
    dmin = min(dmin, distTerrain(p));
    return dmin;
}

vec3 myrefract(vec3 v, vec3 n, float k) {
    float nv = dot(n, v);
    float knormal = (sqrt((k * k - 1.0) / (nv * nv) + 1.0) - 1.0) * nv;
    return normalize(v + knormal * n);
}

float getShortDistance(vec3 src, vec3 dir, float start) {
    float dist, fulldist = start;
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

vec3 lensNormal(vec3 p) {
    if (p.z > -EPSILON) {
        return vec3(0, 0, 1);
    } else {
        return (p - vec3(0, 9, 5)) / 5.5;
    }
}

vec3 estimateNormal(vec3 z) {
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

vec3 illumPhong(vec3 kd, vec3 ks, float shininess, vec3 p, vec3 n,
        vec3 light_pos, vec3 intensity, vec3 eye_dir) {
    vec3 l = normalize(light_pos - p);
    vec3 r = normalize(reflect(-l, n));
    vec3 color = kd * max(dot(l, n), 0);
    color += ks * pow(max(dot(r, -eye_dir), 0.0), shininess);
    return color * intensity;
}

vec3 illumination(vec3 p, vec3 eye_dir, vec3 n) {

    mat3 k;
    float shininess;
    if (distTerrain(p) < EPSILON * 2) {
        if (g_litemode > 0) {
            k = mat3(
                    0.05375, 0.05, 0.06625,
                    0.18275, 0.17, 0.22525,
                    0.332741, 0.328634, 0.346435
            );
            shininess = 30;
        } else if (distHexPrismRep((p - vec3(0, -4, 1)).xzy, vec2(1, 2)) < EPSILON * 2) {
            k = mat3(
                    0.3, 0.15, 0.15,
                    0.7, 0.4, 0.4,
                    0.1, 0.1, 0.1
            );
            k[0] += vec3(0.3, 0.05, 0.05) * sin(g_time + 2.1);
            shininess = 60;
        } else if (distHexPrismRep((p - vec3(0, -3.7, -1)).xzy, vec2(1, 2)) < EPSILON * 2) {
            k = mat3(
                    0.2, 0.2, 0.15,
                    0.5, 0.5, 0.35,
                    0.1, 0.1, 0.1
            );
            k[0] += vec3(0.2, 0.2, 0.05) * sin(g_time);
            shininess = 60;
        } else if (distHexPrismRep((p - vec3(2, -3.85, 0)).xzy, vec2(1, 2)) < EPSILON * 2) {
            k = mat3(
                    0.05, 0.15, 0.3,
                    0.3, 0.4, 0.7,
                    0.1, 0.1, 0.1
            );
            k[0] += vec3(0.05, 0.05, 0.3) * sin(g_time - 2.1);
            shininess = 60;
        } else {
            k = mat3(
                    0.05375, 0.05, 0.06625,
                    0.18275, 0.17, 0.22525,
                    0.332741, 0.328634, 0.346435
            );
            shininess = 30;
        }
    } else if (distMirror(p) < EPSILON * 2 || distLens(p) < EPSILON * 2) {
        k = mat3(
                0, 0, 0,
                0, 0, 0,
                0.7, 0.7, 0.7
        );
        shininess = 30;
    } else if (distRings(p) < EPSILON * 2 || distPillar(p) < EPSILON * 2){
        k = mat3(
                0.19225, 0.19225, 0.19225,
                0.50754, 0.50754, 0.50754,
                0.508273, 0.508273, 0.508273
        );
        shininess = 40;
    } else {
        vec2 dist_lvl = fractalSphere(p - vec3(0, 9, 20), 5);
        if (dist_lvl.x < EPSILON * 2) {
            float alpha = dist_lvl.y / FRACTAL_ITERATIONS;
            k = mat3(
                    0.3 * alpha, 0.1 * (1 - alpha), 0.1 * (1 - alpha),
                    0.5 * alpha + 0.2, 0.2 * (1 - alpha), 0.2 * (1 - alpha),
                    0.5 * alpha + 0.2, 0.3 * (1 - alpha), 0.3 * (1 - alpha)
            );
            shininess = 25;
        } else {
            k = mat3(
                    0, 0.05, 0.05,
                    0.4, 0.5, 0.5,
                    0.04, 0.7, 0.7
            );
            shininess = 25;
        }
    }
    vec3 color = k[0];
    for (int i = 0; i < LIGHTS_NUM; ++i) {
        float d = getShortDistance(lights[i].pos,
                normalize(p + n * EPSILON * 2 - lights[i].pos), EPSILON * 2);
        if (d > length(p - lights[i].pos) - EPSILON * 5) {
            color += illumPhong(k[1], k[2], shininess, p, n,
                    lights[i].pos, lights[i].intensity, eye_dir);
        }
    }
    return color;
}

vec3 get_color(vec3 p, vec3 ray_dir) {
    int ref = 0;
    float k = 1;
    vec3 color = vec3(0, 0, 0);
    vec3 n;
    while (distMirror(p) < EPSILON * 2 && ref < MAX_REFLECTION) {
        n = estimateNormal(p);
        color += illumination(p, ray_dir, n);
        ray_dir = reflect(ray_dir, n);
        float dist = getShortDistance(p, ray_dir, EPSILON * 2);
        k *= 0.8;
        if (dist > MAX_DISTANCE - EPSILON) {
            return color + mix(texture(skybox, -ray_dir).xyz, vec3(0.5, 0.5, 0.5), 1 - k);
        }
        ref++;
        p += ray_dir * dist;
    }
    ref = 0;
    while (distLens(p) < EPSILON * 2 && ref < MAX_REFRACTION) {
        n = lensNormal(p);
        color += illumination(p, ray_dir, n);
        ray_dir = myrefract(ray_dir, n, 1.52);
        float dist = getShortDistance(p, ray_dir, EPSILON * 5);
        k *= 0.9;
        if (dist > MAX_DISTANCE - EPSILON) {
            return color + mix(texture(skybox, -ray_dir).xyz, vec3(0.5, 0.5, 0.5), 1 - k);
        }
        ref++;
        p += ray_dir * dist;
    }
    n = estimateNormal(p);
    return color + mix(illumination(p, ray_dir, n), vec3(0.5, 0.5, 0.5), 1 - k);
}


void main(void) {

    vec2 screenSize = vec2(g_screenWidth, g_screenHeight);
    vec2 coord = fragmentTexCoord * screenSize;
    
    vec3 ray_pos = g_position; 
    vec3 ray_dir = EyeRayDir(coord, screenSize);
    ray_dir = mat3(g_rotate) * ray_dir;
    
    if (g_litemode > 0) {
        FRACTAL_ITERATIONS = 2;
    }
    
    float dist = getShortDistance(ray_pos, ray_dir, 0);
    
    if (dist < MAX_DISTANCE - EPSILON) {
        ray_pos += ray_dir * dist;
        lights[1].pos = vec3(cos(g_time / 4.0) * 4.0, 2, sin(g_time / 4.0) * 4.0);
        fragColor = vec4(get_color(ray_pos, ray_dir), 1);
    } else {
        fragColor = texture(skybox, -ray_dir);
    }
}


