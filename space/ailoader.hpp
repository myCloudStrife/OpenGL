#ifndef SPACE_AILOADER_HPP_
#define SPACE_AILOADER_HPP_

#include "ShaderProgram.h"
#include "util.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <vector>

constexpr unsigned exTEXTURE_COUNT = 4;
constexpr unsigned REAL_TEXTURE_COUNT = 4;

class exMesh {
    GLuint VAO, VBO, EBO;
    GLuint *textureID;
    unsigned drawCount;
public:
    exMesh(aiMesh *mesh, const char *mesh_name) {
        if (mesh->mTextureCoords[1]) {
            fprintf(stderr, "more than 1 texture coords\n");
        }
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // VBO section
        aiVector3D *verteces = mesh->mVertices;
        aiVector3D *coordUV = mesh->mTextureCoords[0];
        aiVector3D *normals = mesh->mNormals;
        aiVector3D *tangents = mesh->mTangents;
        aiVector3D *bitangents = mesh->mBitangents;
        unsigned numVerteces = mesh->mNumVertices;
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        float *dataVBO = new float[numVerteces * 14];
        glm::vec3 minv(1000.0), maxv(-1000.0);
        printf("verteces: %d\n", numVerteces);
        for (unsigned i = 0; i < numVerteces; ++i) {
            minv = glm::min(minv, glm::vec3(verteces[i].x, verteces[i].y, verteces[i].z));
            maxv = glm::max(maxv, glm::vec3(verteces[i].x, verteces[i].y, verteces[i].z));
            dataVBO[i * 14 + 0] = verteces[i].x;
            dataVBO[i * 14 + 1] = verteces[i].y;
            dataVBO[i * 14 + 2] = verteces[i].z;
            dataVBO[i * 14 + 3] = normals[i].x;
            dataVBO[i * 14 + 4] = normals[i].y;
            dataVBO[i * 14 + 5] = normals[i].z;
            dataVBO[i * 14 + 6] = tangents[i].x;
            dataVBO[i * 14 + 7] = tangents[i].y;
            dataVBO[i * 14 + 8] = tangents[i].z;
            dataVBO[i * 14 + 9] = bitangents[i].x;
            dataVBO[i * 14 + 10] = bitangents[i].y;
            dataVBO[i * 14 + 11] = bitangents[i].z;
            dataVBO[i * 14 + 12] = coordUV[i].x;
            dataVBO[i * 14 + 13] = coordUV[i].y;
        }
        printf("max: %f %f %f\n", maxv.x, maxv.y, maxv.z);
        printf("min: %f %f %f\n", minv.x, minv.y, minv.z);
        glBufferData(GL_ARRAY_BUFFER, numVerteces * 14 * sizeof(dataVBO[0]), dataVBO, GL_STATIC_DRAW);
        delete [] dataVBO;

        // EBO section
        aiFace *faces = mesh->mFaces;
        unsigned numFaces = mesh->mNumFaces;
        drawCount = numFaces * 3;
        printf("faces: %d\n", numFaces);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        unsigned *dataEBO = new unsigned[numFaces * 3];
        for (unsigned i = 0; i < numFaces; ++i) {
            dataEBO[i * 3] = faces[i].mIndices[0];
            dataEBO[i * 3 + 1] = faces[i].mIndices[1];
            dataEBO[i * 3 + 2] = faces[i].mIndices[2];
        }
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, numFaces * 3 * sizeof(dataEBO[0]), dataEBO, GL_STATIC_DRAW);
        delete [] dataEBO;

        /*int k = 177;
        glm::vec3 n = normalize(glm::vec3(normals[k].x, normals[k].y, normals[k].z));
        printf("%f %f %f\n", n.x, n.y, n.z);
        glm::vec3 t = normalize(glm::vec3(tangents[k].x, tangents[k].y, tangents[k].z));
        printf("%f %f %f\n", t.x, t.y, t.z);
        t = normalize(t - dot(t, n) * n);
        printf("%f %f %f\n", t.x, t.y, t.z);
        glm::vec3 b = normalize(glm::vec3(bitangents[k].x, bitangents[k].y, bitangents[k].z));
        printf("%f %f %f\n", b.x, b.y, b.z);
        b = normalize(cross(n, t));
        printf("%f %f %f\n", b.x, b.y, b.z);
        printf("---   ---   ---\n");*/


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

        textureID = new GLuint[exTEXTURE_COUNT];
        //glGenTextures(TEXTURE_COUNT, textureID);
        const char *modes[] = {"AO", "Diffuse", "Spec", "Normals"};
        char filename[64];
        for (unsigned i = 0; i < exTEXTURE_COUNT; ++i) {
            //glBindTexture(GL_TEXTURE_2D, textureID[i]);
            int width, height, channel;
            //sprintf(filename, "textures/%s_%s.png", mesh_name, modes[i]);
            sprintf(filename, "textures/%s/%s.dds", mesh_name, modes[i]);
            //unsigned char *data = stbi_load(filename, &width, &height, &channel, 0);
            //printf("hi\n");
            textureID[i] = load_DDS(filename);
            if (!textureID[i]) {
                if (i == 3) {
                    textureID[i] = load_DDS("textures/blue.dds");
                } else {
                    textureID[i] = load_DDS("textures/white.dds");
                }
            }
            //if (channel != 4) {
            //    printf("in %s %d channels\n", filename, channel);
            //}
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            //glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //stbi_image_free(data);
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
    ~exMesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteTextures(exTEXTURE_COUNT, textureID);
    }
};


class exModel3D {
    std::vector<exMesh *> meshes = std::vector<exMesh *>();
public:
    exModel3D(const char *path) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate |
                aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace |
                aiProcess_FlipUVs | aiProcess_GenNormals);
        if(!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
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
        printf("%d meshes\n", scene->mNumMeshes);
        for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
            exMesh *mesh = new exMesh(scene->mMeshes[i], name);
            meshes.push_back(mesh);
        }
        importer.FreeScene();
    }
    static void save(const char *srcpath, const char *dstpath) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(srcpath, aiProcess_Triangulate |
                aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace |
                aiProcess_FlipUVs | aiProcess_GenNormals);
        if(!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
        char name[16] = "";
        int begin = 0, end = 0;
        for (unsigned i = 0; srcpath[i]; ++i) {
            if (srcpath[i] == '/') {
                begin = i;
            } else if (srcpath[i] == '.') {
                end = i;
            }
        }
        strncpy(name, srcpath + begin, end - begin);
        name[end - begin] = '\0';
        FILE *f = fopen(dstpath, "wb");
        char type[8] = "MyType";
        fwrite(type, sizeof(type), 1, f);
        fwrite(&scene->mNumMeshes, sizeof(unsigned), 1, f);
        for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
            aiMesh *mesh = scene->mMeshes[i];
            aiVector3D *verteces = mesh->mVertices;
            aiVector3D *coordUV = mesh->mTextureCoords[0];
            aiVector3D *normals = mesh->mNormals;
            aiVector3D *tangents = mesh->mTangents;
            aiVector3D *bitangents = mesh->mBitangents;
            unsigned numVerteces = mesh->mNumVertices;
            float *dataVBO = new float[numVerteces * 14];
            for (unsigned i = 0; i < numVerteces; ++i) {
                dataVBO[i * 14 + 0] = -verteces[i].x;
                dataVBO[i * 14 + 1] = verteces[i].y;
                dataVBO[i * 14 + 2] = -verteces[i].z;
                dataVBO[i * 14 + 3] = -normals[i].x;
                dataVBO[i * 14 + 4] = normals[i].y;
                dataVBO[i * 14 + 5] = -normals[i].z;
                dataVBO[i * 14 + 6] = -tangents[i].x;
                dataVBO[i * 14 + 7] = tangents[i].y;
                dataVBO[i * 14 + 8] = -tangents[i].z;
                dataVBO[i * 14 + 9] = -bitangents[i].x;
                dataVBO[i * 14 + 10] = bitangents[i].y;
                dataVBO[i * 14 + 11] = -bitangents[i].z;
                dataVBO[i * 14 + 12] = coordUV[i].x;
                dataVBO[i * 14 + 13] = coordUV[i].y;
            }
            fwrite(&numVerteces, sizeof(numVerteces), 1, f);
            fwrite(dataVBO, sizeof(float) * 14, numVerteces, f);
            delete [] dataVBO;
            aiFace *faces = mesh->mFaces;
            unsigned numFaces = mesh->mNumFaces;
            unsigned *dataEBO = new unsigned[numFaces * 3];
            for (unsigned i = 0; i < numFaces; ++i) {
                dataEBO[i * 3] = faces[i].mIndices[0];
                dataEBO[i * 3 + 1] = faces[i].mIndices[1];
                dataEBO[i * 3 + 2] = faces[i].mIndices[2];
            }
            fwrite(&numFaces, sizeof(numFaces), 1, f);
            fwrite(dataEBO, sizeof(unsigned) * 3, numFaces, f);
            delete [] dataEBO;
        }
        fclose(f);
        importer.FreeScene();
    }
    void Draw(ShaderProgram *shader) {
        int i = 0;
        for (exMesh *mesh : meshes) {
            //i++;
            //if (i == 4) {
                mesh->Draw(shader);
            //}
        }
    }
    ~exModel3D() {
        for (exMesh *mesh : meshes) {
            delete mesh;
        }
    }
};

#endif /* SPACE_AILOADER_HPP_ */

