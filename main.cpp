//internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "LiteMath.h"

//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <random>


constexpr double MOVE_SPEED = 3.0;
constexpr double MOVE_SPEED_WITH_SHIFT = 9.0;
constexpr double ROTATE_SPEED = 1.5;

static GLsizei WIDTH = 768, HEIGHT = 512; //размеры окна

using namespace LiteMath;

float3 position(0, 0, 5);
float cam_rot[2] = { 0, 0 };
int mx = 0, my = 0;

void windowResize(GLFWwindow* window, int width, int height) {
    WIDTH  = width;
    HEIGHT = height;
}

static void mouseMove(GLFWwindow* window, double xpos, double ypos) {
    xpos *= -ROTATE_SPEED;
    ypos *= ROTATE_SPEED;

    int x1 = int(xpos);
    int y1 = int(ypos);

    cam_rot[0] -= 0.005f*(y1 - my);
    cam_rot[1] -= 0.005f*(x1 - mx);

    if (cam_rot[0] > M_PI_2) {
        cam_rot[0] = M_PI_2;
    } else if (cam_rot[0] < -M_PI_2) {
        cam_rot[0] = -M_PI_2;
    }

    mx = int(xpos);
    my = int(ypos);
}

void control(GLFWwindow* window) {
	static double lastTime = glfwGetTime(), currentTime, deltaTime;
	double speed = MOVE_SPEED;
	currentTime = glfwGetTime();
	deltaTime = currentTime - lastTime;
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
