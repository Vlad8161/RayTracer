//
// Created by vlad on 4/9/17.
//

#ifndef RAY_TRACING_SCENE_H
#define RAY_TRACING_SCENE_H

#include <glm/glm.hpp>
#include <vector>

class SceneObject {
public:
    virtual glm::vec3 intersection(glm::vec3 ray) = 0;
};

class MeshObject : SceneObject {
public:
    glm::vec3 intersection(glm::vec3 ray) override {
        return glm::vec3();
    }

private:
    std::vector<glm::vec3> mMesh;
    glm::vec3 mPos;
    glm::mat3x3 mTransform;
};

class SphereObject : SceneObject {
public:
    glm::vec3 intersection(glm::vec3 ray) override {
        return glm::vec3();
    }

private:
    glm::vec3 mPos;
    float mRadius;
};

class Lamp {

};

class Scene {
public:
    std::vector<SceneObject> objects;
    std::vector<Lamp> lamps;
    glm::vec3 camPos;
    glm::vec3 camDir;
};

#endif //RAY_TRACING_SCENE_H
