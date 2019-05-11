//internal includes
#include "common.h"
#include "ShaderProgram.h"

//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "myloader.hpp"
#include "textrender.hpp"

static const GLsizei WIDTH = 800, HEIGHT = 600; //размеры окна

constexpr unsigned SHADOW_MAP_WIDTH = 1024;
constexpr unsigned SHADOW_MAP_HEIGHT = 1024;
constexpr float SHADOW_NEAR_PLANE = 0.1;
constexpr float SHADOW_FAR_PLANE = 50.0;

constexpr unsigned SHADOWMODE = 0x0001;
constexpr unsigned NORMALMODE = 0x0002;

constexpr unsigned POINTLIGHT = 0x0004;
constexpr unsigned SUNLIGHT = 0x0008;

static int programMode = NORMALMODE | POINTLIGHT;

using namespace glm;

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

void sceneRender(Model3D *models[], GLuint quadVAO, ShaderProgram *shader, unsigned drawMode) {
    shader->StartUseShader();
    if (drawMode == SHADOWMODE) {
        glViewport(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);
    } else {
        glViewport(0, 0, WIDTH, HEIGHT);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }
    ShaderProgram *drawShader = (drawMode == SHADOWMODE) ? nullptr : shader;

    mat4 model;
    double time = glfwGetTime();

    if (drawMode == NORMALMODE) {
        shader->SetUniform("floorRender", 1);
    }
    model = scale(translate(mat4(1.0f), vec3(0.0f, -10.0f, 0.0f)), vec3(40.0f));
    model = rotate(model, (float) -M_PI_2, vec3(1, 0, 0));
    shader->SetUniform("model", model);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    if (drawMode == NORMALMODE) {
        shader->SetUniform("floorRender", 0);
    }

    model = translate(mat4(1.0f), vec3(15 * sin(time) - 4, -1, 15 * cos(time) - 1));
    model = rotate(scale(model, vec3(0.02)), (float) time, vec3(0, 1, 0));
    model = rotate(model, -0.3f, vec3(1, 0, 0));
    shader->SetUniform("model", model);
    models[0]->Draw(drawShader);

    model = translate(mat4(1.0f), vec3(-15 * sin(time * 0.66) + 7, 4, 15 * cos(time * 0.66) + 3));
    model = rotate(scale(model, vec3(0.02)), radians(180.0f) - (float) (time * 0.66), vec3(0, 1, 0));
    model = rotate(model, 0.3f, vec3(1, 0, 0));
    shader->SetUniform("model", model);
    models[0]->Draw(drawShader);

    model = rotate(scale(translate(mat4(1.0f), vec3(5, 5, 0)), vec3(2.0)), (float) (-time / 5), vec3(-0.7, 0.8, 0.4));
    shader->SetUniform("model", model);
    models[1]->Draw(drawShader);

    model = rotate(scale(translate(mat4(1.0f), vec3(-3, -4, -3)), vec3(1.25)), (float) (-time / 4), vec3(-0.15, -0.33, 0.8));
    shader->SetUniform("model", model);
    models[1]->Draw(drawShader);

    model = rotate(scale(translate(mat4(1.0f), vec3(-10, 1, 8)), vec3(1)), (float) (-time / 3), vec3(0.45, 0.33, -0.4));
    shader->SetUniform("model", model);
    models[1]->Draw(drawShader);

    model = rotate(scale(translate(mat4(1.0f), vec3(16, 0, 13)), vec3(1)), (float) (-time / 3.7), vec3(-0.1, 0.9, -0.4));
    shader->SetUniform("model", model);
    models[1]->Draw(drawShader);

    model = rotate(scale(translate(mat4(1.0f), vec3(-8, 8, -8)), vec3(1.25)), (float) (-time / 4.5), vec3(0.7, 0.2, -0.7));
    shader->SetUniform("model", model);
    models[1]->Draw(drawShader);

    shader->StopUseShader();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if(key == GLFW_KEY_1 && action == GLFW_PRESS) {
        programMode = (programMode | NORMALMODE) & ~SHADOWMODE;
    }
    if(key == GLFW_KEY_2 && action == GLFW_PRESS) {
        programMode = (programMode | SHADOWMODE) & ~NORMALMODE;
    }
    if(key == GLFW_KEY_3 && action == GLFW_PRESS) {
        programMode ^= POINTLIGHT | SUNLIGHT;
    }
}

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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow*  window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL shadow map task", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if(initGL() != 0) {
        return -1;
    }

    //Reset any OpenGL errors which could be present for some reason
    GLenum gl_error = glGetError();
    while (gl_error != GL_NO_ERROR)
        gl_error = glGetError();


    std::unordered_map<GLenum, std::string> shaders;
    shaders[GL_VERTEX_SHADER]   = "shaders/lighting.vs";
    shaders[GL_FRAGMENT_SHADER] = "shaders/lighting.fs";
    ShaderProgram program(shaders);
    GL_CHECK_ERRORS;

    glfwSwapInterval(1); // force 60 frames per second
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    {
    Model3D obj("models/Ship.data");
    Model3D ast("models/Asteroid.data");
    TextRender tr;

    Model3D *models[] = { &obj, &ast };

    GLuint quadVBO;
    GLuint quadVAO;
    {
        float edgePos[] = {
                -1, 1, 0, 0, 0, 1,
                -1, -1, 0, 0, 0, 1,
                1, 1, 0, 0, 0, 1,
                1, -1, 0, 0, 0, 1
        };

        quadVBO = 0;

        glGenBuffers(1, &quadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(GLfloat), (GLfloat*)edgePos, GL_STATIC_DRAW);

        glGenVertexArrays(1, &quadVAO);
        glBindVertexArray(quadVAO);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *) (3 * sizeof(GLfloat)));

        glBindVertexArray(0);
    }

    GL_CHECK_ERRORS;

    vec3 camPos = vec3(20.0f, 30.0f, 50.0f);
    vec3 lightPos = vec3(0.0f, 25.0f, 0.0f);
    vec3 sunDir = normalize(vec3(1.0f, -3.0f, -1.0f));

    mat4 proj = perspective(radians(60.0f), (float) (WIDTH / HEIGHT), 0.1f, 150.0f);
    mat4 view = lookAt(camPos, vec3(0), vec3(0, 1, 0));

    program.StartUseShader();
    program.SetUniform("camPos", camPos);
    program.SetUniform("pl.position", lightPos);
    program.SetUniform("pl.colorIntensity", vec3(500));
    program.SetUniform("sun.direction", sunDir);
    program.SetUniform("sun.colorIntensity", vec3(0.7));
    program.SetUniform("projection", proj);
    program.SetUniform("view", view);

    GLuint shadowFBO, shadowMap;
    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shaders.clear();
    shaders[GL_VERTEX_SHADER] = "shaders/shadow.vs";
    shaders[GL_FRAGMENT_SHADER] = "shaders/shadow.fs";
    ShaderProgram shadowShader(shaders);
    shaders[GL_VERTEX_SHADER] = "shaders/texture.vs";
    shaders[GL_FRAGMENT_SHADER] = "shaders/texture.fs";
    ShaderProgram textureShader(shaders);


    GLuint floorTexture = load_DDS_DXT1("textures/deck.dds");
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        mat4 ShadowViewProj;
        if (programMode & POINTLIGHT) {
            ShadowViewProj = perspective(radians(100.0f), 1.0f, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE) *
                    lookAt(lightPos, vec3(0), vec3(0, 0, -1));
        } else {
            ShadowViewProj = ortho(-30.0f, 30.0f, -30.0f, 30.0f, -20.0f, 20.0f) *
                    lookAt(-sunDir, vec3(0), vec3(0, 1, 0));
        }
        shadowShader.StartUseShader();
        shadowShader.SetUniform("ViewProj", ShadowViewProj);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        sceneRender(models, quadVAO, &shadowShader, SHADOWMODE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (programMode & NORMALMODE) {
            program.StartUseShader();
            program.SetUniform("shadowVP", ShadowViewProj);
            if (programMode & POINTLIGHT) {
                program.SetUniform("lightType", 1);
            } else {
                program.SetUniform("lightType", 0);
            }
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, shadowMap);
            program.SetUniform("texture4", 4);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floorTexture);

            sceneRender(models, quadVAO, &program, NORMALMODE);

        } else {
            glViewport(0, 0, WIDTH, HEIGHT);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            textureShader.StartUseShader();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, shadowMap);
            textureShader.SetUniform("texture0", 0);
            if (programMode & POINTLIGHT) {
                textureShader.SetUniform("near", SHADOW_NEAR_PLANE);
                textureShader.SetUniform("far", SHADOW_FAR_PLANE);
                textureShader.SetUniform("lightType", 1);
            } else {
                textureShader.SetUniform("lightType", 0);
            }
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            textureShader.StopUseShader();
        }

        showFPS(tr);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    shadowShader.Release();
    textureShader.Release();
    glDeleteFramebuffers(1, &shadowFBO);
    glDeleteTextures(1, &shadowMap);
    glDeleteTextures(1, &floorTexture);

    }
    program.Release();

    GL_CHECK_ERRORS;

    //Warning!
    //All models must be destruct before glfwTerminate
    glfwTerminate();
    return 0;
}
