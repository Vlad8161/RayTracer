//
// Created by vlad on 4/9/17.
//

#ifndef RAY_TRACING_SCENE_H
#define RAY_TRACING_SCENE_H

#include <glm/glm.hpp>
#include <vector>

#define EPS (0.0001)

class Intersection {
public:
    Intersection(const glm::vec3 &point = glm::vec3(), const glm::vec3 &norm = glm::vec3()) : point(point),
                                                                                              norm(norm) {}

    glm::vec3 point;
    glm::vec3 norm;
};


class Material {
public:
    Material(float diffusiveIntensity, float diffusiveColorRed, float diffusiveColorGreen, float diffusiveColorBlue)
            : diffusiveIntensity(diffusiveIntensity), diffusiveColorRed(diffusiveColorRed),
              diffusiveColorGreen(diffusiveColorGreen), diffusiveColorBlue(diffusiveColorBlue) {}

    float diffusiveIntensity;
    float diffusiveColorRed;
    float diffusiveColorGreen;
    float diffusiveColorBlue;
};


class SceneObject {
public:
    Material material;

    SceneObject(const Material &material) : material(material) {}

    virtual std::tuple<bool, float, Intersection> intersection(const glm::vec3 &rayFrom, const glm::vec3 &rayDir) = 0;
};


class MeshObject : public SceneObject {
public:
    MeshObject(const Material &material, const std::shared_ptr<std::vector<std::array<glm::vec3, 3>>> &mMesh,
               const glm::vec3 &mPos,
               const glm::mat3 &mTransform) : SceneObject(material), mMesh(mMesh),
                                              mWorldMesh(new std::vector<std::array<glm::vec3, 3>>()), mPos(mPos),
                                              mTransform(mTransform) {
        for (auto &i : *mMesh) {
            auto v1 = i[0] * mTransform + mPos;
            auto v2 = i[1] * mTransform + mPos;
            auto v3 = i[2] * mTransform + mPos;
            std::array<glm::vec3, 3> worldFace = {v1, v2, v3};
            mWorldMesh->push_back(worldFace);
        }
    }

    std::tuple<bool, float, Intersection> intersection(const glm::vec3 &rayFrom, const glm::vec3 &rayDir) override {
        bool retIsIntersect = false;
        float retT = 0.0;
        Intersection retIntersection;

        for (auto &i : *mWorldMesh) {
            auto norm = glm::cross(i[0] - i[1], i[1] - i[2]);
            auto normRayDirDot = glm::dot(norm, rayDir);

            if (fabsf(normRayDirDot) < EPS) {
                continue;
            }

            auto t = -(glm::dot(norm, rayFrom) - glm::dot(norm, i[0])) / glm::dot(norm, rayDir);
            if (t < EPS) {
                continue;
            }

            auto intersectPt = rayFrom + t * rayDir;
            auto a = glm::distance(i[0], i[1]);
            auto b = glm::distance(i[1], i[2]);
            auto c = glm::distance(i[2], i[0]);
            auto aa = glm::distance(i[0], intersectPt);
            auto bb = glm::distance(i[1], intersectPt);
            auto cc = glm::distance(i[2], intersectPt);
            auto p = (a + b + c) / 2;
            auto sqrMain = sqrtf(p * (p - a) * (p - b) * (p - c));
            p = (a + aa + bb) / 2;
            auto sqr1 = sqrtf(p * (p - a) * (p - aa) * (p - bb));
            p = (b + bb + cc) / 2;
            auto sqr2 = sqrtf(p * (p - b) * (p - bb) * (p - cc));
            p = (c + aa + cc) / 2;
            auto sqr3 = sqrtf(p * (p - c) * (p - aa) * (p - cc));
            if (fabsf(sqrMain - (sqr1 + sqr2 + sqr3)) > EPS) {
                continue;
            }

            if (!retIsIntersect || t < retT) {
                retIsIntersect = true;
                retT = t;
                retIntersection.norm = norm;
                retIntersection.point = intersectPt;
            }
        }
        return std::make_tuple(retIsIntersect, retT, Intersection(glm::vec3(), glm::vec3()));
    }

private:
    std::shared_ptr<std::vector<std::array<glm::vec3, 3>>> mMesh;
    std::shared_ptr<std::vector<std::array<glm::vec3, 3>>> mWorldMesh;
    glm::vec3 mPos;
    glm::mat3 mTransform;
};


class SphereObject : public SceneObject {
public:
    SphereObject(const Material &material, const glm::vec3 &mPos, float mRadius)
            : SceneObject(material), mPos(mPos), mRadius(mRadius) {}

    std::tuple<bool, float, Intersection> intersection(const glm::vec3 &rayFrom, const glm::vec3 &rayDir) override {
        return std::make_tuple(false, 0.0, Intersection(glm::vec3(), glm::vec3()));
    }

private:
    glm::vec3 mPos;
    float mRadius;
};


class Lamp {
public:
    Lamp(const glm::vec3 &pos, float intensity) : pos(pos), intensity(intensity) {}

    glm::vec3 pos;
    float intensity;
};


class Scene {
public:
    std::vector<std::shared_ptr<SceneObject>> objects;
    std::vector<std::shared_ptr<Lamp>> lamps;
    glm::vec3 camPos;
    glm::mat3 camMat;
};

#endif //RAY_TRACING_SCENE_H
