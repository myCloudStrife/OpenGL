#ifndef SPACE_TEXTRENDER_HPP_
#define SPACE_TEXTRENDER_HPP_

#include "ShaderProgram.h"
#include <vector>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

class TextRender {
    GLuint textureID, VAO, quadVBO, uvVBO;
    unsigned uvVBOlength = 16;
    ShaderProgram *shader;
public:
    TextRender() {
        std::unordered_map<GLenum, std::string> shaders;
        shaders[GL_VERTEX_SHADER]   = "shaders/text.vs";
        shaders[GL_FRAGMENT_SHADER] = "shaders/text.fs";
        shader = new ShaderProgram(shaders);

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        int width, height, channel;
        unsigned char *data = stbi_load("textures/Text.png", &width, &height, &channel, 0);
        if (channel != 4) {
            printf("in %s %d channels\n", "textures/Text.png", channel);
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &quadVBO);
        glGenBuffers(1, &uvVBO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        GLfloat quad[8] = {
                0.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 1.0f,
                1.0f, 0.0f
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad[0]) * 8, quad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * uvVBOlength, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, 0);
        glVertexAttribDivisor(1, 1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
    }
    void draw(const char *string, float left, float right, float down, float up, glm::vec3 color) {
        unsigned len = strlen(string);

        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        GLfloat *data = new GLfloat[len * 2];
        double letterW = 1.0 / 13, letterH = 76.0 / 512;
        for (unsigned i = 0; i < len; ++i) {
            if (string[i] >= 'A' && string[i] <= 'Z') {
                data[i * 2 + 0] = (string[i] - 'A') % 13 * letterW;
                data[i * 2 + 1] = (string[i] - 'A') / 13 * letterH;
            } else if (string[i] >= 'a' && string[i] <= 'z') {
                data[i * 2 + 0] = (string[i] - 'a') % 13 * letterW;
                data[i * 2 + 1] = (string[i] - 'a') / 13 * letterH;
                data[i * 2 + 1] += 2 * letterH;
            } else if (string[i] >= '0' && string[i] <= '9') {
                data[i * 2 + 0] = (string[i] - '0') % 13 * letterW;
                data[i * 2 + 1] = (string[i] - '0') / 13 * letterH;
                data[i * 2 + 1] += 4 * letterH;
            } else if (string[i] == ' ') {
                data[i * 2 + 0] = 10 * letterW;
                data[i * 2 + 1] = 4 * letterH;
            } else if (string[i] == ':') {
                data[i * 2 + 0] = 11 * letterW;
                data[i * 2 + 1] = 4 * letterH;
            }
        }
        if (len > uvVBOlength) {
            uvVBOlength = len;
            glBufferData(GL_ARRAY_BUFFER, sizeof(data[0]) * 2 * uvVBOlength, data, GL_DYNAMIC_DRAW);
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data[0]) * 2 * len, data);
        }
        delete [] data;

        float fieldWidth = (right - left) / len;

//        glDisable(GL_DEPTH_TEST);

        glBindTexture(GL_TEXTURE_2D, textureID);
        glBindVertexArray(VAO);

        shader->StartUseShader();
        shader->SetUniform("offset", fieldWidth);
        shader->SetUniform("left", left);
        shader->SetUniform("down", down);
        shader->SetUniform("up", up);
        shader->SetUniform("color", color);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, len);
        shader->StopUseShader();

        glBindVertexArray(0);
    }
    ~TextRender() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &uvVBO);
        glDeleteBuffers(1, &quadVBO);
        glDeleteTextures(1, &textureID);
        shader->Release();
        delete shader;
    }
};

#endif /* SPACE_TEXTRENDER_HPP_ */
