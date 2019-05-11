#ifndef SPACE_OBJECT_HPP_
#define SPACE_OBJECT_HPP_

#include "myloader.hpp"
#include <random>
#include <list>
#include <set>
#include <algorithm>
#include <irrKlang.h>

constexpr int DRAW_WITH_INSTANCING = 1;
constexpr int DRAW_WITH_MODEL = 2;

class EnemyShip;

struct shaderData {
    glm::vec4 pos_scale;
    glm::vec4 rotateAxis_angle;
};

struct hiddenData {
    glm::vec3 velocity;
    float rotateSpeed;
};

class Object {
protected:
    Model3D *model;
public:
    glm::vec3 hitBox = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 velocity;
    float rotateSpeed = 0.5;
    glm::vec4 pos_scale;
    glm::vec4 rotateAxis_angle;

    Object(Model3D *m) {
        model = m;
        for (unsigned i = 0; i < m->meshes.size(); ++i) {
            hitBox = glm::max(hitBox, m->meshes[i]->hitBox);
        }
    };
    Object(Model3D *m, glm::vec3 v, glm::vec3 p, float s) : model(m), velocity(v) {
        for (unsigned i = 0; i < m->meshes.size(); ++i) {
            hitBox = glm::max(hitBox, m->meshes[i]->hitBox);
        }
        pos_scale = glm::vec4(p, s);
        hitBox *= s;
        glm::vec3 ra = glm::normalize(glm::vec3(rand(-1.0, 1.0), rand(-1.0, 1.0), rand(-1.0, 1.0)));
        rotateAxis_angle = glm::vec4(ra, rand(0.0, M_PI));
    }
    Object(Model3D *m, glm::vec3 hb, glm::vec3 v, glm::vec4 p_s, glm::vec4 ra_a, float rs)
    : model(m), velocity(v), pos_scale(p_s), rotateAxis_angle(ra_a), rotateSpeed(rs) {
        hitBox = hb * p_s.w;
    }

    void move(float deltaTime) {
        pos_scale += glm::vec4(deltaTime * velocity, 0.0);
        rotateAxis_angle.w += deltaTime * rotateSpeed;
        if (rotateAxis_angle.w > M_PI * 2) {
            rotateAxis_angle.w -= M_PI * 2;
        }
    }
    void draw(ShaderProgram *shader, const glm::mat4 & VP, const glm::vec3 & camPos) {
        shader->SetUniform("VP", VP);
        shader->SetUniform("camPos", camPos);
        shader->SetUniform("uni_pos_scale", pos_scale);
        shader->SetUniform("uni_rotax_angle", rotateAxis_angle);
        model->Draw(shader);
    }
};

class ObjectArray {
    Model3D *model;
    GLuint instancingVBO;
    std::list<unsigned> destroyed = std::list<unsigned>();
    glm::vec3 hitBox = glm::vec3(0.0, 0.0, 0.0);
    std::vector<hiddenData> hid = std::vector<hiddenData>();
public:
    std::vector<shaderData> obj = std::vector<shaderData>();

    ObjectArray(Model3D *m) : model(m) {
        glGenBuffers(1, &instancingVBO);
        for (unsigned i = 0; i < m->meshes.size(); ++i) {
            hitBox = glm::max(hitBox, m->meshes[i]->hitBox);
        }
    }
    ~ObjectArray() {
        glDeleteBuffers(1, &instancingVBO);
    }
    Object operator[](unsigned index) {
        return Object(model, hitBox, hid[index].velocity, obj[index].pos_scale,
                obj[index].rotateAxis_angle, hid[index].rotateSpeed);
    }
    void add(const Object & o) {
        if (!destroyed.size()) {
            obj.push_back({ o.pos_scale, o.rotateAxis_angle });
            hid.push_back({ o.velocity, o.rotateSpeed });
        } else {
            unsigned index = destroyed.front();
            destroyed.pop_front();
            obj[index] = { o.pos_scale, o.rotateAxis_angle };
            hid[index] = { o.velocity, o.rotateSpeed };
        }
    }
    void destroy(unsigned index) {
        obj[index].pos_scale.w = 0;
        destroyed.push_back(index);
    }
    void move(double deltatime) {
        float dt = deltatime;
        for (unsigned i = 0; i < obj.size(); ++i) {
            if (obj[i].pos_scale.w != 0.0) {
                obj[i].pos_scale.x += dt * hid[i].velocity.x;
                obj[i].pos_scale.y += dt * hid[i].velocity.y;
                obj[i].pos_scale.z += dt * hid[i].velocity.z;
                if (obj[i].pos_scale.z > 60.0) {
                    destroy(i);
                }
                obj[i].rotateAxis_angle.w += dt * hid[i].rotateSpeed;
                if (obj[i].rotateAxis_angle.w > M_PI * 2) {
                    obj[i].rotateAxis_angle.w -= M_PI * 2;
                }
            }
        }
    }
    /*int shootBox(const glm::vec3 & pos, const glm::vec3 & dir_range) const {
        glm::vec3 dir = glm::normalize(dir_range);
        float range = glm::length(dir_range);
        float hitBoxlength = glm::length(hitBox);
        for (unsigned i = 0; i < obj.size(); ++i) {
            glm::vec3 point = pos - glm::vec3(obj[i].pos_scale);
            float scale = obj[i].pos_scale.w;
            glm::vec3 hb = hitBox * scale;
            if (scale != 0.0 && glm::length(point) < range + hitBoxlength * scale) {
                glm::vec3 dir2 = dir;
                float range2 = range;
                if (point.x < 0) {
                    point.x = -point.x;
                    dir2.x = -dir2.x;
                }
                if (point.y < 0) {
                    point.y = -point.y;
                    dir2.y = -dir2.y;
                }
                if (point.z < 0) {
                    point.z = -point.z;
                    dir2.z = -dir2.z;
                }
                if (point.x < hb.x && point.y < hb.y && point.z < hb.z) {
                    return i;
                }
                if (point.x < hb.x) {
                    if (dir2.x > 0) {
                        range2 = std::min(range2, (hb.x - point.x) / dir.x);
                    } else {
                        range2 = std::min(range2, (-hb.x - point.x) / dir.x);
                    }
                }
                if (point.y < hb.y) {
                    if (dir2.y > 0) {
                        range2 = std::min(range2, (hb.y - point.y) / dir.y);
                    } else {
                        range2 = std::min(range2, (-hb.y - point.y) / dir.y);
                    }
                }
                if (point.z < hb.z) {
                    if (dir2.z > 0) {
                        range2 = std::min(range2, (hb.z - point.z) / dir.z);
                    } else {
                        range2 = std::min(range2, (-hb.z - point.z) / dir.z);
                    }
                }
                if (point.x > hb.x) {
                    if (dir2.x > 0) {
                        continue;
                    } else if ((hb.x - point.x) / dir.x > range2) {
                        continue;
                    }
                }
                if (point.y > hb.y) {
                    if (dir2.y > 0) {
                        continue;
                    } else if ((hb.y - point.y) / dir.y > range2) {
                        continue;
                    }
                }
                if (point.z > hb.z) {
                    if (dir2.z > 0) {
                        continue;
                    } else if ((hb.z - point.z) / dir.z > range2) {
                        continue;
                    }
                }
                return i;
            }
        }
        return -1;
    }*/

    int shootSphere(const glm::vec3 & pos, const glm::vec3 & dir_range) const {
        float radius = std::max(std::max(hitBox.x, hitBox.y), hitBox.z);
        glm::vec3 dir = glm::normalize(dir_range);
        float range = glm::length(dir_range);
        for (unsigned i = 0; i < obj.size(); ++i) {
            glm::vec3 point = pos - glm::vec3(obj[i].pos_scale);
            float scale = obj[i].pos_scale.w;
            float r = radius * scale;
            float l = glm::length(point);
            if (scale != 0.0 && l < range + r) {
                if (l < r || glm::length(point + dir_range) < r) {
                    return i;
                }
                float need_range = -glm::dot(point, dir);
                float shortest = l * l - need_range * need_range;
                if (shortest < r * r && need_range > 0 && need_range < range) {
                    return i;
                }
            }
        }
        return -1;
    }

    void draw(ShaderProgram *shader) {
        glBindBuffer(GL_ARRAY_BUFFER, instancingVBO);
        glBufferData(GL_ARRAY_BUFFER, obj.size() * sizeof(shaderData), &obj[0], GL_STATIC_DRAW);
        //printf("%d\n", obj.size() - destroyed.size());

        for (unsigned i = 0; i < model->meshes.size(); ++i) {
            glBindVertexArray(model->meshes[i]->VAO);
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4), 0);
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4), (void *) sizeof(glm::vec4));
            glVertexAttribDivisor(6, 1);
            glVertexAttribDivisor(7, 1);
            glBindVertexArray(0);
        }
        shader->SetUniform("source", 1);
        model->DrawInstanced(shader, obj.size());
        shader->SetUniform("source", 0);
    }
};

struct Missile {
    glm::vec3 position;
    glm::vec3 velocity;
    bool operator<(const Missile & m) {
        return this->position.z < m.position.z;
    }
};

class MissileArray {
    unsigned destroyed = 0;
    std::vector<Missile> missiles = std::vector<Missile>();
    //std::vector<glm::vec3> positions = std::vector<glm::vec3>();
    //std::vector<glm::vec3> velocities = std::vector<glm::vec3>();
    ShaderProgram *shader;
    GLuint VBO, VAO;
public:
    MissileArray(ShaderProgram *shaderProgram) {
        shader = shaderProgram;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Missile), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Missile), (void *) sizeof(glm::vec3));

        glBindVertexArray(0);
    }
    ~MissileArray() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    inline Missile & operator[](unsigned index) {
        return missiles[index];
    }
    inline size_t size() {
        return missiles.size();
    }
    void add(const glm::vec3 & pos, const glm::vec3 & vel) {
        if (!destroyed) {
            missiles.push_back({ pos, vel });
            //positions.push_back(pos);
            //velocities.push_back(vel);
        } else {
            //unsigned index = destroyed.front();
            //destroyed.pop_front();
            destroyed--;
            missiles[destroyed].position = pos;
            missiles[destroyed].velocity = vel;
        }
    }
    void destroy(unsigned index) {
        if (index > destroyed) {
            missiles[index] = missiles[destroyed];
        }
        missiles[destroyed] = { glm::vec3(0.0, 0.0, 60.0), glm::vec3(0.0) };
        destroyed++;
        //velocities[index] = glm::vec3(0.0, 0.0, 0.0);
        //destroyed.push_back(index);
    }
    void sort() {
        std::sort(missiles.begin() + destroyed, missiles.end());
    }
    void move(float deltaTime) {
        for (unsigned i = 0; i < missiles.size(); ++i) {
            missiles[i].position += deltaTime * missiles[i].velocity;
            if (missiles[i].position.z < -500.0 || missiles[i].position.z > 65.0) {
                destroy(i);
            }
        }
        /*for (unsigned i = 0; i < positions.size(); ++i) {
            positions[i] += deltaTime * velocities[i];
            if (positions[i].z < -500.0 || positions[i].z > 70.0) {
                destroy(i);
            }
        }*/
    }
    std::pair<std::set<unsigned>, std::set<unsigned>>
    check_hits(const ObjectArray & objs, float deltaTime) {
        std::pair<std::set<unsigned>, std::set<unsigned>> ret;
        for (unsigned i = 0; i < missiles.size(); ++i) {
            if (abs(missiles[i].velocity.z) > 0.01) {
                //int target = objs.shootBox(positions[i], velocities[i] * deltaTime);
                int target = objs.shootSphere(missiles[i].position,
                        missiles[i].velocity * deltaTime);
                if (target >= 0) {
                    ret.first.insert(target);
                    ret.second.insert(i);
                    //destroy(i);
                }
            }
        }
        /*for (unsigned i = 0; i < positions.size(); ++i) {
            if (abs(velocities[i].z) > 0.01) {
                //int target = objs.shootBox(positions[i], velocities[i] * deltaTime);
                int target = objs.shootSphere(positions[i], velocities[i] * deltaTime);
                if (target >= 0) {
                    ret.insert(target);
                    destroy(i);
                }
            }
        }*/
        return ret;
    }

    std::set<unsigned> check_hits(const EnemyShip & es, float deltaTime);

    void draw(const glm::mat4 & VP, const glm::vec3 & camPos, const glm::vec3 & color) {
        shader->StartUseShader();
        shader->SetUniform("VP", VP);
        shader->SetUniform("camPos", camPos);
        shader->SetUniform("color", color);
        sort();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, missiles.size() * sizeof(Missile),
                &missiles[0], GL_STATIC_DRAW);
        //glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        //glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3),
        //        &velocities[0], GL_STATIC_DRAW);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, missiles.size());
        shader->StopUseShader();
    }
};


struct Weapon {
    glm::vec3 position;
    float cooldown;
};

class EnemyShip: public Object {
    std::vector<Weapon> weapons = std::vector<Weapon>();
    float speed;
public:
    unsigned health = 5;
    EnemyShip(Model3D *m, glm::vec3 pos, float scale, float speed, const std::vector<Weapon> & weaps)
            : Object(m), speed(speed) {
        pos_scale = glm::vec4(pos, scale);
        hitBox *= scale;
        velocity = glm::vec3(glm::normalize(glm::vec2(rand(-1.0, 1.0), rand(-1.0, 1.0))), 0.5);
        rotateAxis_angle = glm::vec4(0.0, M_PI, 0.0, 0.0);
        weapons = weaps;
    }
    void move(float deltaTime, const glm::vec3 & target) {
        glm::vec3 targetDir = glm::normalize(target - glm::vec3(pos_scale));
        float angleY = atan(-targetDir.x / targetDir.z);
        float angleX = asin(targetDir.y);
        if (targetDir.z > 0) {
            angleY = M_PI + angleY;
        }
        rotateAxis_angle.x = rotateAxis_angle.x * 0.97f + angleX * 0.03f;
        rotateAxis_angle.y = rotateAxis_angle.y * 0.97f + angleY * 0.03f;
        float x = rotateAxis_angle.x, y = rotateAxis_angle.y;
        glm::vec3 vel = glm::vec3(sin(y) * cos(x), sin(x), -cos(y) * cos(x)) * speed;
        pos_scale += glm::vec4(deltaTime * vel, 0.0);
        for (unsigned i = 0; i < weapons.size(); ++i) {
            weapons[i].cooldown -= deltaTime;
        }
    }
    void draw(ShaderProgram *shader, const glm::mat4 & VP, const glm::vec3 & camPos) {
        glm::mat4 model = glm::scale(
                glm::translate(glm::mat4(1.0), glm::vec3(pos_scale)),
                glm::vec3(pos_scale.w));
        //model = glm::rotate(model, myShip->rotateAxis_angle.z, vec3(0.0, 0.0, 1.0));
        model = glm::rotate(model, -rotateAxis_angle.y, glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, rotateAxis_angle.x, glm::vec3(1.0, 0.0, 0.0));

        shader->SetUniform("VP", VP);
        shader->SetUniform("camPos", camPos);
        shader->SetUniform("source", DRAW_WITH_MODEL);
        shader->SetUniform("uni_model", model);
        this->model->Draw(shader);
        shader->SetUniform("source", 0);
    }
    void shoot(MissileArray *ma, const glm::vec3 & target, irrklang::ISoundEngine* engine) {
        for (unsigned i = 0; i < weapons.size(); ++i) {
            if (weapons[i].cooldown < 0) {
                weapons[i].cooldown = 2.0;
                glm::vec3 pos = weapons[i].position;
                pos = rotateX(pos, rotateAxis_angle.x);
                pos = rotateY(pos, -rotateAxis_angle.y);
                pos += glm::vec3(pos_scale);
                glm::vec3 vel = glm::normalize(target - pos) * 80.0f;
                ma->add(pos, vel);
                engine->play3D("audio/laser.wav",
                        irrklang::vec3df(pos.x, pos.y, pos.z) / 100.0);
            }
        }
    }
    bool check_hit(const glm::vec3 & pos, const glm::vec3 & dir_range) const {
        glm::vec3 dir = glm::normalize(dir_range);
        float range = glm::length(dir_range);
        glm::vec3 point = pos - glm::vec3(pos_scale);
        if (glm::length(point) - range < glm::length(hitBox)) {
            dir = rotateY(dir, rotateAxis_angle.y);
            dir = rotateX(dir, -rotateAxis_angle.x);
            point = rotateY(point, rotateAxis_angle.y);
            point = rotateX(point, -rotateAxis_angle.x);
            if (point.x < 0) {
                point.x = -point.x;
                dir.x = -dir.x;
            }
            if (point.y < 0) {
                point.y = -point.y;
                dir.y = -dir.y;
            }
            if (point.z < 0) {
                point.z = -point.z;
                dir.z = -dir.z;
            }
            if (point.x < hitBox.x && point.y < hitBox.y && point.z < hitBox.z) {
                return true;
            }
            if ((point.x > hitBox.x && dir.x > 0) || (point.y > hitBox.y && dir.y > 0) ||
                    (point.z > hitBox.z && dir.z > 0)) {
                return false;
            }
            float rangein = 0.0;
            if (dir.x > 0) {
                range = std::min(range, (hitBox.x - point.x) / dir.x);
            } else {
                range = std::min(range, (-hitBox.x - point.x) / dir.x);
                if (point.x > hitBox.x) {
                    rangein = std::max(rangein, (hitBox.x - point.x) / dir.x);
                }
            }
            if (dir.y > 0) {
                range = std::min(range, (hitBox.y - point.y) / dir.y);
            } else {
                range = std::min(range, (-hitBox.y - point.y) / dir.y);
                if (point.y > hitBox.y) {
                    rangein = std::max(rangein, (hitBox.y - point.y) / dir.y);
                }
            }
            if (dir.z > 0) {
                range = std::min(range, (hitBox.z - point.z) / dir.z);
            } else {
                range = std::min(range, (-hitBox.z - point.z) / dir.z);
                if (point.z > hitBox.z) {
                    rangein = std::max(rangein, (hitBox.z - point.z) / dir.z);
                }
            }
            if (rangein < range) {
                return true;
            }
        }
        return false;
    }
};

std::set<unsigned> MissileArray::check_hits(const EnemyShip & es, float deltaTime) {
    std::set<unsigned> ret;
    for (unsigned i = 0; i < missiles.size(); ++i) {
        if (abs(missiles[i].velocity.z) > 0.01) {
            if (es.check_hit(missiles[i].position, missiles[i].velocity * deltaTime)) {
                ret.insert(i);
            }
        }
    }
    return ret;
}

struct Explosion: public Object {
    unsigned timer;
    float startTime;
    Explosion(const Object & obj, unsigned timer, float startTime)
            : Object(obj), timer(timer), startTime(startTime) {}
};

struct ShipExplosion: public EnemyShip {
    unsigned timer;
    float startTime;
    ShipExplosion(const EnemyShip & obj, unsigned timer, float startTime)
            : EnemyShip(obj), timer(timer), startTime(startTime) {}
};

class Explosions {
    ShaderProgram *destructShader, *boomShader;
    GLuint textureID;
    std::list<Explosion> expls = std::list<Explosion>();
    std::list<ShipExplosion> shipexpls = std::list<ShipExplosion>();
public:
    Explosions() {
        std::unordered_map<GLenum, std::string> shaders;
        shaders[GL_VERTEX_SHADER]   = "shaders/explode.vs";
        shaders[GL_GEOMETRY_SHADER] = "shaders/explode.gs";
        shaders[GL_FRAGMENT_SHADER] = "shaders/object.fs";
        destructShader = new ShaderProgram(shaders);
        shaders[GL_VERTEX_SHADER]   = "shaders/boom.vs";
        shaders[GL_GEOMETRY_SHADER] = "shaders/boom.gs";
        shaders[GL_FRAGMENT_SHADER] = "shaders/texture.fs";
        boomShader = new ShaderProgram(shaders);
        textureID = load_DDS("textures/boom.dds");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    ~Explosions() {
        glDeleteTextures(1, &textureID);
        destructShader->Release();
        boomShader->Release();
        delete destructShader;
        delete boomShader;
    }
    void add(const Object & obj, unsigned timer, float startTime) {
        expls.push_back(Explosion(obj, timer, startTime));
    }
    void add(const EnemyShip & obj, unsigned timer, float startTime) {
        shipexpls.push_back(ShipExplosion(obj, timer, startTime));
    }
    void draw(float currTime, float deltaTime, const glm::mat4 & VP, const glm::vec3 & camPos) {
        destructShader->StartUseShader();
        for (auto iter = expls.begin(); iter != expls.end(); ++iter) {
            iter->move(deltaTime);
            destructShader->SetUniform("time", currTime - iter->startTime);
            iter->draw(destructShader, VP, camPos);
            iter->timer--;
            if (!iter->timer) {
                iter = expls.erase(iter);
                iter--;
            }
        }
        for (auto iter = shipexpls.begin(); iter != shipexpls.end(); ++iter) {
            destructShader->SetUniform("time", currTime - iter->startTime);
            iter->draw(destructShader, VP, camPos);
        }
        boomShader->StartUseShader();
        boomShader->SetUniform("VP", VP);
        boomShader->SetUniform("camPos", camPos);
        boomShader->SetUniform("color", glm::vec3(1.0));
        GL_CHECK_ERRORS;
        for (auto iter = shipexpls.begin(); iter != shipexpls.end(); ++iter) {
            boomShader->SetUniform("pos", glm::vec3(iter->pos_scale));
            boomShader->SetUniform("timer", (int) iter->timer);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            boomShader->SetUniform("texture0", 0);
            GL_CHECK_ERRORS;
            glDrawArrays(GL_POINTS, 0, 1);
            GL_CHECK_ERRORS;
            iter->timer--;
            if (!iter->timer) {
                iter = shipexpls.erase(iter);
                iter--;
            }
        }
        GL_CHECK_ERRORS;
        boomShader->StartUseShader();
    }
};

#endif /* SPACE_OBJECT_HPP_ */
