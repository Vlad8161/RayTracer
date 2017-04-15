//
// Created by vlad on 4/9/17.
//

#ifndef RAY_TRACING_SCENE_H
#define RAY_TRACING_SCENE_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "ImageBitmap.h"

#define EPS (0.0001)

typedef struct _Hit {
    glm::vec3 point;
    glm::vec3 norm;

    _Hit(
            const glm::vec3 &point = glm::vec3(),
            const glm::vec3 &norm = glm::vec3())
            : point(point), norm(norm) {}
} Hit;


typedef struct _Material {
    float diffusiveIntensity;
    float diffusiveColorRed;
    float diffusiveColorGreen;
    float diffusiveColorBlue;

    _Material(float diffusiveIntensity = 0.0, float diffusiveColorRed = 0.0, float diffusiveColorGreen = 0.0, float diffusiveColorBlue = 0.0)
            : diffusiveIntensity(diffusiveIntensity), diffusiveColorRed(diffusiveColorRed),
              diffusiveColorGreen(diffusiveColorGreen), diffusiveColorBlue(diffusiveColorBlue) {}
} Material;


typedef struct _Triangle {
    glm::vec3 p1;
    glm::vec3 p2;
    glm::vec3 p3;
    std::shared_ptr<Material> material;

    _Triangle(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, const std::shared_ptr<Material> &material)
            : p1(p1), p2(p2), p3(p3), material(material) {}
} Triangle;


typedef struct _Sphere {
    glm::vec3 center;
    float radius;
    std::shared_ptr<Material> material;

    _Sphere(const glm::vec3 &center, float radius, const std::shared_ptr<Material> &material) : center(center),
                                                                                                radius(radius),
                                                                                                material(material) {}
} Sphere;


typedef struct _Lamp {
    glm::vec3 pos;
    float intensity;

    _Lamp(const glm::vec3 &pos, float intensity) : pos(pos), intensity(intensity) {}
} Lamp;


typedef struct _Scene {
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Triangle>> triangles;
    std::vector<std::shared_ptr<Sphere>> spheres;
    std::vector<std::shared_ptr<Lamp>> lamps;
    glm::vec3 camPos;
    glm::mat3 camMat;
} Scene;


void loadScene(Scene &outScene, const std::string &pathToScene);

void renderScene(ImageBitmap &outImg, const Scene &scene);

#endif //RAY_TRACING_SCENE_H
