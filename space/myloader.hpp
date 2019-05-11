#ifndef SPACE_MYLOADER_HPP_
#define SPACE_MYLOADER_HPP_

#include "ShaderProgram.h"
#include "util.hpp"

#include <vector>

constexpr unsigned TEXTURE_COUNT = 4;

class Mesh {
    GLuint VBO, EBO;
    GLuint *textureID;
    unsigned drawCount;
public:
    glm::vec3 hitBox = glm::vec3(0.0, 0.0, 0.0);
    GLuint VAO;
    Mesh(FILE *f, const char *mesh_name) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // VBO section
        unsigned numVerteces;
        fread(&numVerteces, sizeof(numVerteces), 1, f);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        float *dataVBO = new float[numVerteces * 14];
        fread(dataVBO, sizeof(float) * 14, numVerteces, f);
        for (unsigned i = 0; i < numVerteces; ++i) {
            //len = std::max(len, glm::length(glm::vec3(dataVBO[i * 14 + 0], dataVBO[i * 14 + 1], dataVBO[i * 14 + 2])));
            if (abs(dataVBO[i * 14 + 0]) > hitBox.x) {
                hitBox.x = abs(dataVBO[i * 14 + 0]);
            }
            if (abs(dataVBO[i * 14 + 1]) > hitBox.y) {
                hitBox.y = abs(dataVBO[i * 14 + 1]);
            }
            if (abs(dataVBO[i * 14 + 2]) > hitBox.z) {
                hitBox.z = abs(dataVBO[i * 14 + 2]);
            }
        }
        glBufferData(GL_ARRAY_BUFFER, numVerteces * 14 * sizeof(dataVBO[0]), dataVBO, GL_STATIC_DRAW);
        delete [] dataVBO;

        // EBO section
        unsigned numFaces;
        fread(&numFaces, sizeof(numFaces), 1, f);
        drawCount = numFaces * 3;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        unsigned *dataEBO = new unsigned[numFaces * 3];
        fread(dataEBO, sizeof(float) * 3, numFaces, f);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, numFaces * 3 * sizeof(dataEBO[0]), dataEBO, GL_STATIC_DRAW);
        delete [] dataEBO;

        // vertex coords
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), 0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *) (3 * sizeof(float)));
        // vertex tangents
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *) (6 * sizeof(float)));
        // vertex bitangents
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *) (9 * sizeof(float)));
        // vertex texture coords
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *) (12 * sizeof(float)));
        glBindVertexArray(0);

        textureID = new GLuint[TEXTURE_COUNT];
        const char *modes[] = {"AO", "Diffuse", "Spec", "Normals"};
        char filename[64];
        for (unsigned i = 0; i < TEXTURE_COUNT; ++i) {
            sprintf(filename, "textures/%s/%s.dds", mesh_name, modes[i]);
            textureID[i] = load_DDS(filename);
            if (!textureID[i]) {
                if (i == 3) {
                    textureID[i] = load_DDS("textures/blue.dds");
                } else {
                    textureID[i] = load_DDS("textures/white.dds");
                }
            }
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    void Draw(ShaderProgram *shader) {
        if (shader) {
            char uniformName[16];
            for (unsigned i = 0; i < TEXTURE_COUNT; ++i) {
                sprintf(uniformName, "texture%d", i);
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, textureID[i]);
                shader->SetUniform(uniformName, (int) i);
            }
        }
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT, 0);
        //glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void DrawInstanced(ShaderProgram *shader, unsigned count) {
        if (shader) {
            char uniformName[16];
            for (unsigned i = 0; i < TEXTURE_COUNT; ++i) {
                sprintf(uniformName, "texture%d", i);
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, textureID[i]);
                shader->SetUniform(uniformName, (int) i);
            }
        }
        glBindVertexArray(VAO);
        glDrawElementsInstanced(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT, 0, count);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    ~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteTextures(TEXTURE_COUNT, textureID);
    }
};


class Model3D {
public:
    std::vector<Mesh *> meshes = std::vector<Mesh *>();
    Model3D(const char *path) {
        FILE *f = fopen(path, "rb");
        if (!f) {
            fprintf(stderr, "can not open %s\n", path);
        }
        char header[8];
        fread(header, sizeof(header), 1, f);
        if (strncmp(header, "MyType", 8)) {
            fprintf(stderr, "file %s with incorrect type\n", path);
        }
        unsigned meshNumber;
        fread(&meshNumber, sizeof(meshNumber), 1, f);
        char name[16] = "";
        int begin = 0, end = 0;
        for (unsigned i = 0; path[i]; ++i) {
            if (path[i] == '/') {
                begin = i;
            } else if (path[i] == '.') {
                end = i;
            }
        }
        strncpy(name, path + begin, end - begin);
        name[end - begin] = '\0';
        for (unsigned i = 0; i < meshNumber; ++i) {
            Mesh *mesh = new Mesh(f, name);
            meshes.push_back(mesh);
        }
    }
    void Draw(ShaderProgram *shader) {
        for (Mesh *mesh : meshes) {
            mesh->Draw(shader);
        }
    }
    void DrawInstanced(ShaderProgram *shader, unsigned count) {
        for (Mesh *mesh : meshes) {
            mesh->DrawInstanced(shader, count);
        }
    }
    ~Model3D() {
        for (Mesh *mesh : meshes) {
            delete mesh;
        }
    }
};

#endif /* SPACE_MYLOADER_HPP_ */
