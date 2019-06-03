//internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "LiteMath.h"

//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <random>
#include <vector>
#include <string>

constexpr double MOVE_SPEED = 3.0;
constexpr double MOVE_SPEED_WITH_SHIFT = 9.0;
constexpr double ROTATE_SPEED = 1.5;

static GLsizei WIDTH = 512, HEIGHT = 512; //размеры окна

using namespace LiteMath;

float3 position(0, 2, 7);
float cam_rot[2] = { 0, 0 };
int litemode = 0;

void windowResize(GLFWwindow* window, int width, int height) {
    WIDTH  = width;
    HEIGHT = height;
}

static void mouseMove(GLFWwindow* window, double xpos, double ypos) {
    xpos *= -ROTATE_SPEED;
    ypos *= ROTATE_SPEED;

    static float mx = xpos, my = ypos;

    cam_rot[0] -= 0.005f*(ypos - my);
    cam_rot[1] -= 0.005f*(xpos - mx);

    if (cam_rot[0] > M_PI_2) {
        cam_rot[0] = M_PI_2;
    } else if (cam_rot[0] < -M_PI_2) {
        cam_rot[0] = -M_PI_2;
    }

    mx = xpos;
    my = ypos;
}

void control(GLFWwindow* window) {
    static double lastTime = glfwGetTime(), currentTime, deltaTime;
    double speed = MOVE_SPEED;
    currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        litemode = 1;
    }
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
        litemode = 0;
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        speed = MOVE_SPEED_WITH_SHIFT;
    } else {
        speed = MOVE_SPEED;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position.z -= deltaTime * speed * cos(cam_rot[1]);
        position.x += deltaTime * speed * sin(cam_rot[1]);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        position.z += deltaTime * speed * cos(cam_rot[1]);
        position.x -= deltaTime * speed * sin(cam_rot[1]);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position.x += deltaTime * speed * cos(cam_rot[1]);
        position.z += deltaTime * speed * sin(cam_rot[1]);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position.x -= deltaTime * speed * cos(cam_rot[1]);
        position.z -= deltaTime * speed * sin(cam_rot[1]);
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        position.y += deltaTime * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        position.y -= deltaTime * speed;
    }
    lastTime = currentTime;
}

int initGL() {
    int res = 0;
    //грузим функции opengl через glad
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    std::cout << "Vendor: "   << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version: "  << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL: "     << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    return 0;
}

unsigned char * load_tga_custom(const char *t_file, int & width, int & height) {
    // Below variables to hold data we will need.
    FILE *_file;
    long size;
    int color;
    unsigned char header[18];

    // Open texture file in RB mode (Read/Binary)
    _file = fopen(t_file, "rb");

    // If unavailable, then return 0
    if (!_file) {
        return 0;
    }

    // Otherwise, get header from TGA
    fread(header, 1, sizeof(header), _file);

    // If not RGB image, return 0
    if (header[2] != 2 || header[16] != 24) {
        printf("here %d %d\n", header[2], header[16]);
        return 0;
    }

    width = header[13] * 256 + header[12];
    height = header[15] * 256 + header[14];

    // Compute how many bits per texture pixel this TGA has available.
    // Convert this data to channels amount, e.g. 3 or 4 channels (RGB or RGBA)
    color = header[16] / 8;

    // Get array size to hold image. We compute total amount of pixels, i.e. width * height, after that we
    // need to allocate space for each pixel, so we multiply previous result with bits per pixel variable
    size = width * height * color;

    // After we know the size of image data array, we declare it
    unsigned char *image = new unsigned char[sizeof(unsigned char) * size];

    // Read pixel data from image to our image array
    fread(image, sizeof(unsigned char), size, _file);

    // Close file after done
    fclose(_file);
    return image;
}

GLuint loadCubemap(std::vector<std::string> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = load_tga_custom(faces[i].c_str(), width, height);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
            delete [] data;
        } else {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            delete [] data;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

int main(int argc, char** argv) {
    if (!glfwInit()) {
        return -1;
    }

    //запрашиваем контекст opengl версии 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL ray marching task",
            nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetCursorPosCallback (window, mouseMove);
    glfwSetWindowSizeCallback(window, windowResize);

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (initGL() != 0) {
        return -1;
    }

    //Reset any OpenGL errors which could be present for some reason
    GLenum gl_error = glGetError();
    while (gl_error != GL_NO_ERROR) {
        gl_error = glGetError();
    }

    std::vector<std::string> faces = {
        "my_darkskies_ft.tga",
        "my_darkskies_bk.tga",
        "my_darkskies_dn.tga",
        "my_darkskies_up.tga",
        "my_darkskies_rt.tga",
        "my_darkskies_lf.tga"
    };
    GLuint cubemapTexture = loadCubemap(faces);

    //создание шейдерной программы из двух файлов с исходниками шейдеров
    //используется класс-обертка ShaderProgram
    std::unordered_map<GLenum, std::string> shaders;
    shaders[GL_VERTEX_SHADER]   = "vertex.glsl";
    shaders[GL_FRAGMENT_SHADER] = "fragment.glsl";
    ShaderProgram program(shaders);
    GL_CHECK_ERRORS;

    glfwSwapInterval(1); // force 60 frames per second

    //Создаем и загружаем геометрию поверхности
    GLuint g_vertexBufferObject;
    GLuint g_vertexArrayObject;
    {
        float quadPos[] =
        {
                -1.0f,  1.0f, // v0 - top left corner
                -1.0f, -1.0f, // v1 - bottom left corner
                1.0f,  1.0f,  // v2 - top right corner
                1.0f, -1.0f   // v3 - bottom right corner
        };

        g_vertexBufferObject = 0;
        GLuint vertexLocation = 0; // simple layout, assume have only positions at location = 0

        glGenBuffers(1, &g_vertexBufferObject);                                                        GL_CHECK_ERRORS;
        glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferObject);                                           GL_CHECK_ERRORS;
        glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), (GLfloat*)quadPos, GL_STATIC_DRAW);     GL_CHECK_ERRORS;

        glGenVertexArrays(1, &g_vertexArrayObject);                                                    GL_CHECK_ERRORS;
        glBindVertexArray(g_vertexArrayObject);                                                        GL_CHECK_ERRORS;

        glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferObject);                                           GL_CHECK_ERRORS;
        glEnableVertexAttribArray(vertexLocation);                                                     GL_CHECK_ERRORS;
        glVertexAttribPointer(vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);                            GL_CHECK_ERRORS;

        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    }

    //цикл обработки сообщений и отрисовки сцены каждый кадр
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        control(window);

        //очищаем экран каждый кадр
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);               GL_CHECK_ERRORS;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GL_CHECK_ERRORS;

        program.StartUseShader();                           GL_CHECK_ERRORS;

        float4x4 camRotMatrix = mul(rotate_Y_4x4(-cam_rot[1]),
                rotate_X_4x4(+cam_rot[0]));

        float time = glfwGetTime();

        program.SetUniform("g_litemode", litemode);
        program.SetUniform("g_time", time);
        program.SetUniform("g_position", position);
        program.SetUniform("g_rotate", camRotMatrix);
        program.SetUniform("g_screenWidth", WIDTH);
        program.SetUniform("g_screenHeight", HEIGHT);

        // очистка и заполнение экрана цветом
        //
        glViewport(0, 0, WIDTH, HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // draw call
        //
        glBindVertexArray(g_vertexArrayObject);
        GL_CHECK_ERRORS;
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        GL_CHECK_ERRORS;  // The last parameter of glDrawArrays is equal to VS invocations

        program.StopUseShader();

        glfwSwapBuffers(window);
    }

    //очищаем vboи vao перед закрытием программы
    //
    glDeleteVertexArrays(1, &g_vertexArrayObject);
    glDeleteBuffers(1, &g_vertexBufferObject);
    glfwTerminate();
    return 0;
}
