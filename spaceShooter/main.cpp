//internal includes
#include "common.h"
#include "ShaderProgram.h"

//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <random>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <irrKlang.h>

//#pragma comment(lib, "irrKlang.lib")

#include "myloader.hpp"
#include "textrender.hpp"
#include "object.hpp"
#include "particles.hpp"

using namespace glm;
using namespace std;
using namespace irrklang;

static const GLsizei WIDTH = 800, HEIGHT = 600; //размеры окна

constexpr double ASTEROID_RATE = 0.007;
constexpr double ENEMY_RATE = 0.0005;
constexpr double ENEMY_SPAWN_MAX_TIME = 10.0;
constexpr double HEALTH_RECOVERY = 0.016;
constexpr double MAX_HEALTH = 100.0;

double currentTime = 0, prevTime;

vec3 camPos = vec3(0.0f, 20.0f, 60.0f);

Object *myShip;
MissileArray *myMissiles;
vector<ObjectArray *> all_asteroids;
list<EnemyShip> *enemylist;

ISoundEngine* engine;

double Health = MAX_HEALTH;
unsigned Score = 0;

void control(GLFWwindow* window) {
    bool left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    if (!left & right) {
        myShip->velocity.x = std::min(myShip->velocity.x + 0.5, 20.0);
        if (myShip->pos_scale.x > 30.0) {
            myShip->velocity.x = 0;
        }
        myShip->rotateAxis_angle.z = std::max(myShip->rotateAxis_angle.z - 0.005, -0.2);
        myShip->rotateAxis_angle.y = std::max(myShip->rotateAxis_angle.y - 0.005, -0.2);
    } else if (left & !right) {
        myShip->velocity.x = std::max(myShip->velocity.x - 0.5, -20.0);
        if (myShip->pos_scale.x < -30.0) {
            myShip->velocity.x = 0;
        }
        myShip->rotateAxis_angle.z = std::min(myShip->rotateAxis_angle.z + 0.005, 0.2);
        myShip->rotateAxis_angle.y = std::min(myShip->rotateAxis_angle.y + 0.005, 0.2);
    } else {
        myShip->velocity.x *= 0.93;
        myShip->rotateAxis_angle.z *= 0.98;
        myShip->rotateAxis_angle.y *= 0.98;
    }
}

void mouse_callback(GLFWwindow* window, int button, int action, int mode) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        //engine->play2D("audio/laser.wav");
        engine->play3D("audio/laser.wav", vec3df(myShip->pos_scale.x, 0, 0) / 100.0);
        vec3 position = vec3(0.0, -2.0, -11.0);
        position = rotateZ(position, myShip->rotateAxis_angle.z);
        position = rotateY(position, myShip->rotateAxis_angle.y);
        position += vec3(myShip->pos_scale);
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        x /= WIDTH * 0.5;
        x -= 1.0;
        y /= HEIGHT * 0.5;
        y -= 1.0;

        vec3 ray_target = vec3(x * 500.0 * 0.577350269, y * -500.0 * 0.577350269, -500.0);
        vec3 target = ray_target + camPos;
        for (int i = 0; i < all_asteroids.size(); ++i) {
            int index = all_asteroids[i]->shootSphere(camPos, ray_target);
            if (index >= 0) {
                float z = all_asteroids[i]->obj[index].pos_scale.z;
                if (target.z < z && z < -15.0) {
                    target = vec3(all_asteroids[i]->obj[index].pos_scale);
                }
            }
        }
        for (const EnemyShip & es : *enemylist) {
            if (es.check_hit(camPos, ray_target) && target.z < es.pos_scale.z &&
                    es.pos_scale.z < -15.0) {
                target = vec3(es.pos_scale);
            }
        }
        vec3 velocity = normalize(target - position) * 200.0f;
        myMissiles->add(position, velocity);
    }
}

bool check_hit(vec3 point) {
    if (abs(point.z) < 15) {
        point -= vec3(myShip->pos_scale);
        point = rotateZ(point, -myShip->rotateAxis_angle.z);
        point = rotateY(point, -myShip->rotateAxis_angle.y);
        if (abs(point.z) < 13 && abs(point.x) < 4 && abs(point.y) < 3) {
            return true;
        }
        if (point.z > 0 && point.z < 13 && abs(point.y) < 1.0 && abs(point.x) < 10) {
            return true;
        }
    }
    return false;
}

void showFPS(TextRender & tr) {
    static unsigned sec = 0, frames = 0, len = 1;
    static char fps[10] = "1";
    double time = glfwGetTime();
    frames++;
    if ((unsigned) time > sec) {
        sprintf(fps, "%u", frames);
        len = strlen(fps);
        frames = 0;
        sec = (unsigned) time;
    }
    tr.draw(fps, -1.0, -1.0 + len * 0.15, 0.7, 1.0, vec3(1.0, 1.0, 0.0));
}

class Background {
    GLuint VAO;
    GLuint VBO;
    GLuint texture;
    ShaderProgram shader;
public:
    Background() {
        std::unordered_map<GLenum, std::string> shaders;
        shaders[GL_VERTEX_SHADER]   = "shaders/texture.vs";
        shaders[GL_FRAGMENT_SHADER] = "shaders/texture.fs";
        shader = ShaderProgram(shaders);
        GLfloat quadPos[] = {
                -1, 1, 0, 1,
                -1, -1, 0, 0,
                1, 1, 1, 1,
                1, -1, 1, 0
        };
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), quadPos, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *) 8);

        glBindVertexArray(0);

        texture = load_DDS("textures/space.dds");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    ~Background() {
        shader.Release();
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteTextures(1, &texture);
    }
    void draw() {
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(VAO);
        shader.StartUseShader();
        shader.SetUniform("color", vec3(1.0, 1.0, 1.0));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader.SetUniform("texture0", 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        shader.StopUseShader();
        glEnable(GL_DEPTH_TEST);
    }
};

class Aim {
    GLuint texture;
    unsigned damaged = 0;
    ShaderProgram shader;
public:
    Aim() {
        std::unordered_map<GLenum, std::string> shaders;
        shaders[GL_VERTEX_SHADER]   = "shaders/aim.vs";
        shaders[GL_GEOMETRY_SHADER] = "shaders/aim.gs";
        shaders[GL_FRAGMENT_SHADER] = "shaders/texture.fs";
        shader = ShaderProgram(shaders);

        texture = load_DDS("textures/aim.dds");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    ~Aim() {
        shader.Release();
        glDeleteTextures(1, &texture);
    }
    void hit() {
        damaged = 10;
    }
    void draw(GLFWwindow *window) {
        glDisable(GL_DEPTH_TEST);
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        x /= WIDTH * 0.5;
        x -= 1.0;
        y /= HEIGHT * 0.5;
        y -= 1.0;
        y *= -1.0;

        shader.StartUseShader();
        shader.SetUniform("coord", vec2(x, y));
        if (!damaged) {
            shader.SetUniform("color", vec3(1.0, 1.0, 1.0));
        } else {
            shader.SetUniform("color", vec3(1.0, 0.5, 0.5));
            damaged--;
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader.SetUniform("texture0", 0);
        glDrawArrays(GL_POINTS, 0, 1);
        shader.StopUseShader();
        glEnable(GL_DEPTH_TEST);
    }
};

int initGL() {
    int res = 0;
    //грузим функции opengl через glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    std::cout << "Vendor: "   << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version: "  << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL: "     << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    return 0;
}

int main(int argc, char** argv) {
    if(!glfwInit()) {
        return -1;
    }

    //запрашиваем контекст opengl версии 3.3
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow*  window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL basic sample", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_FALSE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    if(initGL() != 0) {
        return -1;
    }

    //Reset any OpenGL errors which could be present for some reason
    GLenum gl_error = glGetError();
    while (gl_error != GL_NO_ERROR)
        gl_error = glGetError();

    engine = createIrrKlangDevice();
    engine->setListenerPosition(vec3df(camPos.x,camPos.y,camPos.z) / 100.0, vec3df(0,0,1));

    std::unordered_map<GLenum, std::string> shaders;
    shaders[GL_VERTEX_SHADER]   = "shaders/vertex.glsl";
    shaders[GL_FRAGMENT_SHADER] = "shaders/fragment.glsl";
    ShaderProgram program(shaders);
    GL_CHECK_ERRORS;

    srand(clock());
    glfwSwapInterval(1); // force 60 frames per second
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    {
    Model3D ship1_model("source/Ship1.data");
    Model3D ship2_model("source/Ship5.data");
    Model3D ship3_model("source/Ship3.data");
    Model3D ship4_model("source/Ship4.data");
    Model3D ast1_model("source/Asteroid1.data");
    Model3D ast2_model("source/Asteroid2.data");
    //exModel3D sh222("source/Ship5.obj");
    //exModel3D::save("source/Ship4.obj", "source/Ship4.data");
    TextRender tr;

    shaders[GL_VERTEX_SHADER]   = "shaders/object.vs";
    shaders[GL_FRAGMENT_SHADER] = "shaders/object.fs";
    ShaderProgram objectShader(shaders);

    /*shaders[GL_VERTEX_SHADER]   = "shaders/explode.vs";
    shaders[GL_GEOMETRY_SHADER] = "shaders/explode.gs";
    ShaderProgram explodeShader(shaders);*/

    shaders[GL_VERTEX_SHADER]   = "shaders/missile.vs";
    shaders[GL_GEOMETRY_SHADER] = "shaders/missile.gs";
    shaders[GL_FRAGMENT_SHADER] = "shaders/missile.fs";
    ShaderProgram missileShader(shaders);

    ObjectArray asteroids1(&ast1_model);
    ObjectArray asteroids2(&ast2_model);
    Particles particles;
    Background bg;
    Aim aim;
    myShip = new Object(&ship1_model, vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 0.0), 0.035);
    myShip->rotateAxis_angle = vec4(0.0, 0.0, 0.0, 0.0);
    myMissiles = new MissileArray(&missileShader);
    MissileArray enemyMissiles(&missileShader);
    Explosions explosions;
    list<EnemyShip> enemies;
    enemylist = &enemies;

    all_asteroids.push_back(&asteroids1);
    all_asteroids.push_back(&asteroids2);

    /*GLuint VBO;
    GLuint VAO;
    {
        float trianglePos[] = {
                1, 1, 0,
                0, 0, 0,
                1, 0, 0
        };

        VBO = 0;
        GLuint vertexLocation = 0;

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 3 * 3 * sizeof(GLfloat), (GLfloat*)trianglePos, GL_STATIC_DRAW);

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(vertexLocation);
        glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glBindVertexArray(0);
    }*/
    //GLuint texture = create_texture("test.dds");
    //glBindTexture(GL_TEXTURE_2D, texture);

    GL_CHECK_ERRORS;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    mat4 proj = perspective(radians(60.0f), (float) (width / height), 5.0f, 500.0f);
    mat4 view(1.0f);
    view = translate(view, -camPos);

    /*printf("%f %f %f %f\n", view[0][0], view[1][0], view[2][0], view[3][0]);
    printf("%f %f %f %f\n", view[0][1], view[1][1], view[2][1], view[3][1]);
    printf("%f %f %f %f\n", view[0][2], view[1][2], view[2][2], view[3][2]);
    printf("%f %f %f %f\n", view[0][3], view[1][3], view[2][3], view[3][3]);*/

    program.StartUseShader();
    program.SetUniform("camPos", camPos);
    program.SetUniform("projection", proj);
    program.SetUniform("view", view);

    mat4 VP = proj * view;

    objectShader.StartUseShader();
    objectShader.SetUniform("camPos", camPos);
    objectShader.SetUniform("VP", VP);

    float last_enemy_spawn_time = 0.0;

    engine->play2D("audio/Combat.mp3", true);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        prevTime = currentTime;
        currentTime = glfwGetTime();
        float deltaTime = currentTime - prevTime;

        control(window);

        if (rand() < ASTEROID_RATE * RAND_MAX) {
            vec3 pos(rand(-200.0, 200), rand(-100.0, 200.0), -450.0f);
            vec3 vel = normalize(vec3(myShip->pos_scale) - pos) * (float) rand(5.0, 20.0);
            if (rand() & 1) {
                Object ast(&ast1_model, vel, pos, rand(0.5, 2.5));
                asteroids1.add(ast);
            } else {
                Object ast(&ast2_model, vel, pos, rand(1.0, 3.0));
                asteroids2.add(ast);
            }
        }

        if (rand() < ENEMY_RATE * RAND_MAX ||
                currentTime > last_enemy_spawn_time + ENEMY_SPAWN_MAX_TIME) {
            vec3 pos(rand(-200.0, 200), rand(-100.0, 200.0), -450.0f);
            int type = rand() % 3;
            last_enemy_spawn_time = currentTime;
            if (type == 0) {
                vector<Weapon> weap = {
                        { vec3(0.0, -1.85, -18.5), 2.0 }
                };
                enemies.push_back(EnemyShip(&ship2_model, pos, 1.5, 30.0, weap));
            } else if (type == 1) {
                vector<Weapon> weap = {
                        { vec3(11.0, -1.85, -5.0), 2.0 },
                        { vec3(-11.0, -1.85, -5.0), 2.0 }
                };
                enemies.push_back(EnemyShip(&ship4_model, pos, 0.75, 30.0, weap));
            } else {
                vector<Weapon> weap = {
                        { vec3(5.7, -2.85, -20.0), 1.5 },
                        { vec3(-5.7, -2.85, -20.0), 2.0 },
                        { vec3(5.7, 2.85, -20.0), 2.5 },
                        { vec3(-5.7, 2.85, -20.0), 3.0 }
                };
                enemies.push_back(EnemyShip(&ship3_model, pos, 20.0, 30.0, weap));
            }
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        bg.draw();

        myShip->move(deltaTime);
        mat4 model = scale(translate(mat4(1.0), vec3(myShip->pos_scale)), vec3(myShip->pos_scale.w));
        model = rotate(model, myShip->rotateAxis_angle.x, vec3(1.0, 0.0, 0.0));
        model = rotate(model, myShip->rotateAxis_angle.z, vec3(0.0, 0.0, 1.0));
        model = rotate(model, myShip->rotateAxis_angle.y + (float) M_PI_2, vec3(0.0, 1.0, 0.0));

        objectShader.StartUseShader();
        objectShader.SetUniform("source", DRAW_WITH_MODEL);
        objectShader.SetUniform("uni_model", model);
        myShip->draw(&objectShader, VP, camPos);
        objectShader.SetUniform("source", 0);


        for (auto iter = enemies.begin(); iter != enemies.end(); ++iter) {
            if (iter->pos_scale.z < -100.0) {
                iter->move(deltaTime, vec3(myShip->pos_scale));
                iter->shoot(&enemyMissiles, vec3(myShip->pos_scale), engine);
            } else if (iter->pos_scale.z < 60.0) {
                iter->move(deltaTime, vec3(iter->pos_scale) + iter->velocity);
            } else {
                iter = enemies.erase(iter);
                iter--;
            }
        }

        for (auto iter = enemies.begin(); iter != enemies.end(); ++iter) {
            iter->draw(&objectShader, VP, camPos);
            auto s = myMissiles->check_hits(*iter, deltaTime);
            if (s.size()) {
                aim.hit();
                for (unsigned index : s) {
                    vec3 pos = (*myMissiles)[index].position;
                    engine->play3D("audio/hit.wav", vec3df(pos.x, pos.y, pos.z) / 100.0);
                    particles.add(pos, (*myMissiles)[index].velocity, ADD_MY_MISSILE);
                    myMissiles->destroy(index);
                }
                iter->health--;
                if (!iter->health) {
                    Score += 10;
                    engine->play3D("audio/boom.mp3",
                            vec3df(iter->pos_scale.x, iter->pos_scale.y,iter->pos_scale.z) / 100.0);
                    explosions.add(*iter, 60, prevTime);
                    iter = enemies.erase(iter);
                    iter--;
                }
            }
        }

        asteroids1.move(currentTime - prevTime);
        for (unsigned i = 0; i < asteroids1.obj.size(); ++i) {
            bool destroyed = (asteroids1.obj[i].pos_scale.w == 0.0);
            if (!destroyed && check_hit(vec3(asteroids1.obj[i].pos_scale))) {
                engine->play3D("audio/asteroid.wav",
                        vec3df(asteroids1.obj[i].pos_scale.x, 0.0, 0.0) / 100.0);
                explosions.add(asteroids1[i], 60, prevTime);
                asteroids1.destroy(i);
                Health -= 10;
            }
        }
        asteroids1.draw(&objectShader);
        asteroids2.move(currentTime - prevTime);
        for (unsigned i = 0; i < asteroids2.obj.size(); ++i) {
            bool destroyed = (asteroids2.obj[i].pos_scale.w == 0.0);
            if (!destroyed && check_hit(vec3(asteroids2.obj[i].pos_scale))) {
                engine->play3D("audio/asteroid.wav",
                        vec3df(asteroids2.obj[i].pos_scale.x, 0.0, 0.0) / 100.0);
                explosions.add(asteroids2[i], 60, prevTime);
                asteroids2.destroy(i);
                Health -= 10;
            }
        }
        asteroids2.draw(&objectShader);
        objectShader.StopUseShader();

        auto hits = myMissiles->check_hits(asteroids1, currentTime - prevTime);
        for (unsigned index : hits.first) {
            aim.hit();
            Object obj(asteroids1[index]);
            engine->play3D("audio/asteroid.wav",
                    vec3df(obj.pos_scale.x, obj.pos_scale.y, obj.pos_scale.z) / 100.0);
            explosions.add(obj, 60, prevTime);
            particles.add(vec3(obj.pos_scale), obj.velocity, ADD_ASTEROID);
            asteroids1.destroy(index);
            Score += 2;
        }
        for (unsigned index : hits.second) {
            vec3 pos = (*myMissiles)[index].position;
            engine->play3D("audio/hit.wav", vec3df(pos.x, pos.y, pos.z) / 100.0);
            particles.add(pos, (*myMissiles)[index].velocity, ADD_MY_MISSILE);
            myMissiles->destroy(index);
        }
        hits = myMissiles->check_hits(asteroids2, currentTime - prevTime);
        for (unsigned index : hits.first) {
            aim.hit();
            Object obj(asteroids2[index]);
            engine->play3D("audio/asteroid.wav",
                    vec3df(obj.pos_scale.x, obj.pos_scale.y, obj.pos_scale.z) / 100.0);
            explosions.add(obj, 60, prevTime);
            particles.add(vec3(obj.pos_scale), obj.velocity, ADD_ASTEROID);
            asteroids2.destroy(index);
            Score += 2;
        }
        for (unsigned index : hits.second) {
            vec3 pos = (*myMissiles)[index].position;
            engine->play3D("audio/hit.wav", vec3df(pos.x, pos.y, pos.z) / 100.0);
            particles.add(pos, (*myMissiles)[index].velocity, ADD_MY_MISSILE);
            myMissiles->destroy(index);
        }

        myMissiles->move(deltaTime);

        for (unsigned i = 0; i < enemyMissiles.size(); ++i) {
            if (check_hit(enemyMissiles[i].position)) {
                vec3 pos = enemyMissiles[i].position;
                engine->play3D("audio/hit.wav", vec3df(pos.x, pos.y, pos.z) / 100.0);
                particles.add(pos, enemyMissiles[i].velocity, ADD_ENEMY_MISSILE);
                enemyMissiles.destroy(i);
                Health -= 1.0;
            }
        }

        enemyMissiles.move(deltaTime);

        explosions.draw(currentTime, currentTime - prevTime, VP, camPos);
        myMissiles->draw(VP, camPos, vec3(0.0, 1.0, 0.0));
        enemyMissiles.draw(VP, camPos, vec3(1.0, 0.0, 0.0));

        //glBindVertexArray(VAO);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        particles.draw(currentTime - prevTime, VP, camPos);

        aim.draw(window);

        //tr.draw("ScreenSaver", -0.75, 0.75, 0.3, 0.6, vec3(1.0));
        //tr.draw("really", -0.12, 0.12, 0.15, 0.25, vec3(1.0));

        Health += HEALTH_RECOVERY;
        if (Health > MAX_HEALTH) {
            Health = MAX_HEALTH;
        } else if (Health < 1.0) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        char str[20];
        sprintf(str, "Health: %03d", (int) Health);
        tr.draw(str, -0.97, -0.15, 0.8, 0.95, vec3(0.0, 1.0, 0.0));
        sprintf(str, "Score: %06d", Score);
        tr.draw(str, 0.0, 0.97, 0.80, 0.95, vec3(1.0, 1.0, 0.0));
        //showFPS(tr);

        glfwSwapBuffers(window);
    }

    //glDeleteVertexArrays(1, &VAO);
    //glDeleteBuffers(1, &VBO);
    delete myMissiles;
    delete myShip;
    }
    program.Release();

    GL_CHECK_ERRORS;
    engine->drop();
    //Warning!
    //All models must be destruct before glfwTerminate
    glfwTerminate();
    return 0;
}
