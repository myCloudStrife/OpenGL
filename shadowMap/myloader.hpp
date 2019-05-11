#ifndef SPACE_MYLOADER_HPP_
#define SPACE_MYLOADER_HPP_

#include "ShaderProgram.h"

#include <vector>

constexpr unsigned TEXTURE_COUNT = 4;
constexpr unsigned REAL_TEXTURE_COUNT = 4;

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1

GLuint load_DDS_DXT1(const char *path) {
    unsigned char header[124];
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "%s could not be opened.\n", path);
        return 0;
    }
    unsigned signature;
    fread(&signature, sizeof(signature), 1, f);
    if (signature != 0x20534444) { // "DDS " signature
        fclose(f);
        fprintf(stderr, "%s has not dds format.\n", path);
        return 0;
    }
    fread(header, sizeof(header), 1, f);
    unsigned height = *(unsigned *) (header + 8);
    unsigned width = *(unsigned *) (header + 12);
    unsigned linearSize = *(unsigned *) (header + 16);
    unsigned mipMapCount = *(unsigned *) (header + 24);

    unsigned bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
    unsigned char *buffer = new unsigned char [bufsize];
    fread(buffer, 1, bufsize, f);
    fclose(f);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    unsigned offset = 0;
    for (unsigned level = 0; level < mipMapCount; ++level) {
        unsigned size = ((width + 3) / 4) * ((height + 3) / 4) * 8;
        glCompressedTexImage2D(GL_TEXTURE_2D, level,
                GL_COMPRESSED_RGB_S3TC_DXT1_EXT, width, height, 0, size,
                buffer + offset);
        offset += size;
        width  /= 2;
        height /= 2;
        if(width < 1) {
            width = 1;
        }
        if(height < 1) {
            height = 1;
        }
    }

    delete [] buffer;
    return textureID;
}


class Mesh {
    GLuint VAO, VBO, EBO;
    GLuint *textureID;
    unsigned drawCount;
public:
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
            sprintf(filename, "textures/%s/%s_%s.dds", mesh_name, mesh_name, modes[i]);
            textureID[i] = load_DDS_DXT1(filename);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    void Draw(ShaderProgram *shader) {
        if (shader) {
            char uniformName[16];
            for (unsigned i = 0; i < REAL_TEXTURE_COUNT; ++i) {
                sprintf(uniformName, "texture%d", i);
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, textureID[i]);
                shader->SetUniform(uniformName, (int) i);
            }
        }
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT, 0);
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
    std::vector<Mesh *> meshes = std::vector<Mesh *>();
public:
    Model3D(const char *path) {
        FILE *f = fopen(path, "rb");
        char header[8];
        fread(header, sizeof(header), 1, f);
        if (strncmp(header, "MyType", 8)) {
            fprintf(stderr, "file %s with incorrect type", path);
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
    ~Model3D() {
        for (Mesh *mesh : meshes) {
            delete mesh;
        }
    }
};

#endif /* SPACE_MYLOADER_HPP_ */
