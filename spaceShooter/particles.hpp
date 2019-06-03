#ifndef SPACE_PARTICLES_HPP_
#define SPACE_PARTICLES_HPP_

#include "ShaderProgram.h"
#include "util.hpp"

constexpr unsigned MAX_PARTICLES = 1000;
constexpr float PARTICLE_SIMPLE = 0.0f;
constexpr float PARTICLE_STARDUST = 1.0f;
constexpr float PARTICLE_MISSILE = 2.0f;
constexpr float PARTICLE_ASTEROID = 3.0f;
constexpr int ADD_ASTEROID = 1;
constexpr int ADD_MY_MISSILE = 2;
constexpr int ADD_ENEMY_MISSILE = 3;


struct Particle {
    glm::vec4 color_type;
    glm::vec4 pos_size;
    glm::vec4 vel_lifetime;
};

class Particles {
    bool isFirst = true;
    unsigned currentState = 0;
    unsigned used = 0;
    GLuint VAO[2];
    GLuint VBO[2];
    GLuint transformFeedback[2];
    GLuint textureID[2];
    ShaderProgram *updateShader, *drawShader;
public:
    Particles() {
        std::unordered_map<GLenum, std::string> shaders;
        shaders[GL_VERTEX_SHADER]   = "shaders/update_particles.vs";
        shaders[GL_GEOMETRY_SHADER] = "shaders/update_particles.gs";
        updateShader = new ShaderProgram(shaders);

        const GLchar* Varyings[3] = { "color_type", "pos_size", "vel_lifetime" };
        glTransformFeedbackVaryings(updateShader->GetProgram(), 3, Varyings, GL_INTERLEAVED_ATTRIBS);
        updateShader->reLink();

        shaders[GL_VERTEX_SHADER]   = "shaders/draw_particles.vs";
        shaders[GL_GEOMETRY_SHADER] = "shaders/draw_particles.gs";
        shaders[GL_FRAGMENT_SHADER] = "shaders/draw_particles.fs";
        drawShader = new ShaderProgram(shaders);

        glCreateVertexArrays(2, VAO);
        glCreateBuffers(2, VBO);
        glCreateTransformFeedbacks(2, transformFeedback);

        Particle *particles = new Particle[MAX_PARTICLES];
        for (unsigned i = 0; i < 200; ++i) {
            particles[i].color_type = glm::vec4(rand(0.9, 1.0),rand(0.9, 1.0), rand(0.9, 1.0),
                    PARTICLE_STARDUST);
            particles[i].pos_size = glm::vec4(rand(-200.0, 200.0), rand(-200.0, 200.0),
                    rand(-600.0, 0.0), rand(0.1, 0.7));
            particles[i].vel_lifetime = glm::vec4(0.0, 0.0, 100.0, 0.0);
        }
        glBindVertexArray(0);
        for (unsigned i = 0; i < 2 ; ++i) {
            glBindVertexArray(VAO[i]);

            glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformFeedback[i]);
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, VBO[i]);
            glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * MAX_PARTICLES, particles, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), 0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *) 16);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *) 32);
        }
        glBindVertexArray(0);
        delete [] particles;

        textureID[0] = load_DDS("textures/particle.dds");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        float *randomData = new float[128 * 128 * 3];
        for (int i = 0; i < 128 * 128 * 3; ++i) {
            randomData[i] = rand(0.0, 1.0);
        }
        glGenTextures(1, &textureID[1]);
        glBindTexture(GL_TEXTURE_2D, textureID[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_FLOAT, randomData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        delete [] randomData;
    }
    ~Particles() {
        updateShader->Release();
        drawShader->Release();
        delete updateShader;
        delete drawShader;
        glDeleteBuffers(2, VBO);
        glDeleteVertexArrays(2, VAO);
        glDeleteTransformFeedbacks(2, transformFeedback);
        glDeleteTextures(2, textureID);
    }
    void add(glm::vec3 point, glm::vec3 vel, int type) {
        glBindBuffer(GL_ARRAY_BUFFER, VBO[currentState]);
        if (type == ADD_ASTEROID) {
            Particle p = {
                    glm::vec4(0.5, 0.5, 0.5, PARTICLE_ASTEROID),
                    glm::vec4(point, 0.1),
                    glm::vec4(vel, 0.2)
            };
            glBufferSubData(GL_ARRAY_BUFFER, used * sizeof(Particle), sizeof(Particle), &p);
        } else if (type == ADD_MY_MISSILE) {
            glm::vec3 dir = normalize(vel);
            Particle p = {
                    glm::vec4(0.1, 1.0, 0.1, PARTICLE_MISSILE),
                    glm::vec4(point + dir * 2.0f, 0.1),
                    glm::vec4(-dir * 30.0f, 0.1)
            };
            glBufferSubData(GL_ARRAY_BUFFER, used * sizeof(Particle), sizeof(Particle), &p);
        } else {
            glm::vec3 dir = normalize(vel);
            Particle p = {
                    glm::vec4(1.0, 0.1, 0.1, PARTICLE_MISSILE),
                    glm::vec4(point + dir * 2.0f, 0.1),
                    glm::vec4(-dir * 30.0f, 0.1)
            };
            glBufferSubData(GL_ARRAY_BUFFER, used * sizeof(Particle), sizeof(Particle), &p);
        }
        used++;
    }
    void draw(float deltaTime, const glm::mat4 & VP, const glm::vec3 camPos) {
        glBindVertexArray(VAO[currentState]);
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformFeedback[currentState ^ 1]);

        updateShader->StartUseShader();
        updateShader->SetUniform("deltaTime", deltaTime);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID[1]);
        updateShader->SetUniform("texture0", 0);

        glEnable(GL_RASTERIZER_DISCARD);
        glBeginTransformFeedback(GL_POINTS);
        if (isFirst) {
            glDrawArrays(GL_POINTS, 0, 200);
            isFirst = false;
        } else {
            glDrawTransformFeedback(GL_POINTS, transformFeedback[currentState]);
        }
        glEndTransformFeedback();
        glDisable(GL_RASTERIZER_DISCARD);

        glBindVertexArray(VAO[currentState ^ 1]);
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformFeedback[currentState ^ 1]);

        drawShader->StartUseShader();
        drawShader->SetUniform("VP", VP);
        drawShader->SetUniform("camPos", camPos);
        glBindTexture(GL_TEXTURE_2D, textureID[0]);
        drawShader->SetUniform("texture0", 0);
        glDrawTransformFeedback(GL_POINTS, transformFeedback[currentState ^ 1]);
        drawShader->StopUseShader();

        used = 0;
        currentState ^= 1;
    }
};


#endif /* SPACE_PARTICLES_HPP_ */
