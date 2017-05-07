//
// Created by vlad on 4/9/17.
//

#ifndef RAY_TRACING_SCENE_H
#define RAY_TRACING_SCENE_H

#include "config.h"

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <iostream>
#include "lib/lodepng.h"
#include "image_bitmap.h"
#include "tex_image.h"
#include "synchronized_queue.h"

class OpenClExecutor;

typedef struct _Material {
    glm::vec3 color;
    std::shared_ptr<tex_image> texImage;
    float diffusiveFactor;
    float specularFactor;
    float specularHardness;
    float reflectionFactor;
    float refractionFactor;
    float refractionHardness;
    bool textured;

    _Material(const glm::vec3 &color = glm::vec3(0.0f, 0.0f, 0.0f),
              const std::shared_ptr<tex_image> &texImage = std::shared_ptr<tex_image>(),
              float diffusiveFactor = 0.0f,
              float specularFactor = 0.0f,
              float specularHardness = 0.0f,
              float reflectionFactor = 0.0f,
              float refractionFactor = 0.0f,
              float refractionHardness = 0.0f,
              bool textured = false
    ) : color(color), texImage(texImage), diffusiveFactor(diffusiveFactor),
        specularFactor(specularFactor), specularHardness(specularHardness),
        reflectionFactor(reflectionFactor), refractionFactor(refractionFactor),
        refractionHardness(refractionHardness), textured(textured) {}
} Material;


typedef struct _Hit {
    std::shared_ptr<Material> mtl;
    glm::vec3 point;
    glm::vec3 norm;
    glm::vec3 color;
    float t;
    bool isHit;

    _Hit(bool isHit,
         const std::shared_ptr<Material> &mtl = std::shared_ptr<Material>(),
         const glm::vec3 &point = glm::vec3(),
         const glm::vec3 &norm = glm::vec3(),
         const glm::vec3 &color = glm::vec3(),
         float t = 0.0f
    ) : mtl(mtl), point(point), norm(norm), color(color), t(t), isHit(isHit) {}
} Hit;


typedef struct _SphereHit {
    glm::vec3 point;
    glm::vec3 norm;
    float t;
    bool isHit;

    _SphereHit(
            bool isHit,
            float t = 0.0f,
            const glm::vec3 &point = glm::vec3(),
            const glm::vec3 &norm = glm::vec3()
    ) : point(point), norm(norm), t(t), isHit(isHit) {}
} SphereHit;


typedef struct _TriangleHit {
    glm::vec3 point;
    glm::vec3 norm;
    float t;
    float u;
    float v;
    bool isHit;

    _TriangleHit(
            bool isHit,
            float t = 0.0f,
            float u = 0.0f,
            float v = 0.0f,
            const glm::vec3 &point = glm::vec3(),
            const glm::vec3 &norm = glm::vec3()
    ) : point(point), norm(norm), t(t), u(u), v(v), isHit(isHit) {}
} TriangleHit;


typedef struct _Triangle {
    glm::vec3 p;
    glm::vec3 e1;
    glm::vec3 e2;
    glm::vec2 uvStart;
    glm::vec2 uvU;
    glm::vec2 uvV;
    glm::vec3 norm;
    std::shared_ptr<Material> material;

    _Triangle(
            const glm::vec3 &p,
            const glm::vec3 &e1,
            const glm::vec3 &e2,
            const glm::vec2 &uvStart,
            const glm::vec2 &uvU,
            const glm::vec2 &uvV,
            const glm::vec3 &norm,
            const std::shared_ptr<Material> &material)
            : p(p), e1(e1), e2(e2), uvStart(uvStart), uvU(uvU), uvV(uvV), norm(norm), material(material) {}
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
    float distance;

    _Lamp(const glm::vec3 &pos, float intensity, float distance) : pos(pos), intensity(intensity), distance(distance) {}
} Lamp;


typedef struct _Scene {
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Triangle>> triangles;
    std::vector<std::shared_ptr<Sphere>> spheres;
    std::vector<std::shared_ptr<Lamp>> lamps;
    glm::vec3 camPos;
    glm::mat3 camMat;
    glm::vec3 worldHorizonColor;
    glm::vec3 worldAmbientColor;
    float worldAmbientFactor;
} Scene;


typedef struct _RayData {
    float p_x;
    float p_y;
    float p_z;
    float d_x;
    float d_y;
    float d_z;

    _RayData(
            float p_x, float p_y, float p_z,
            float d_x, float d_y, float d_z)
            : p_x(p_x), p_y(p_y), p_z(p_z)
            , d_x(d_x), d_y(d_y), d_z(d_z) {}
} RayData;


void
loadScene(
        Scene &outScene,
        const std::string &pathToScene
);


#endif //RAY_TRACING_SCENE_H
