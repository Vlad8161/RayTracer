//
// Created by vlad on 4/15/17.
//

#include <fstream>
#include "Scene.h"
#include "json.h"

#define AO_RAYS_COUNT (30)

using Json = nlohmann::json;

Hit
computeHit(
        const Triangle &triangle,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir,
        const Material &mtl
);

Hit
computeHit(
        const Sphere &sphere,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir,
        const Material &mtl
);

glm::vec3
computeDiffusiveLight(
        const Scene &scene,
        const glm::vec3 &pt,
        const glm::vec3 &norm,
        const glm::vec3 &rayDir,
        const float k,
        const glm::vec3 &color
);

glm::vec3
computeAmbientOcclusion(
        const Scene &scene,
        const glm::vec3 &pt,
        const glm::vec3 &norm,
        const glm::vec3 &rayDir,
        const glm::vec3 color
);

glm::vec3
traceRay(
        const Scene &scene,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir,
        bool log
);


Hit
computeHit(
        const Triangle &triangle,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir,
        const Material& mtl
) {
    auto e1 = triangle.p2 - triangle.p1;
    auto e2 = triangle.p3 - triangle.p1;
    auto q = rayFrom - triangle.p1;
    auto det = glm::dot(rayDir, glm::cross(e2, e1));

    if (fabsf(det) < EPS) {
        return Hit(false);
    }

    auto dett = glm::dot(q, glm::cross(e1, e2));
    auto t = dett / det;
    if (t < EPS) {
        return Hit(false);
    }

    auto detu = glm::dot(rayDir, glm::cross(e2, q));
    auto detv = glm::dot(rayDir, glm::cross(q, e1));
    auto u = detu / det;
    auto v = detv / det;
    if (u < 0.0 || v < 0.0 || v + u > 1.0) {
        return Hit(false);
    }

    auto hitPt = rayFrom + t * rayDir;
    glm::vec3 hitColor;
    if (mtl.texImage) {
        glm::vec2 texPos = triangle.uv1 + (triangle.uv2 - triangle.uv1) * u + (triangle.uv3 - triangle.uv1) * v;
        glm::vec4 tex = mtl.texImage->get((unsigned int) texPos.x, (unsigned int) texPos.y);
        glm::vec3 texRgb(tex.r, tex.g, tex.b);
        hitColor = (mtl.diffusiveColor * (1 - tex.a)) + (texRgb * tex.a);
    } else {
        hitColor = mtl.diffusiveColor;
    }
    return Hit(true, t, hitPt, glm::cross(e1, e2), hitColor);
}

Hit
computeHit(
        const Sphere &sphere,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir,
        const Material& mtl
) {
    glm::vec3 v = rayFrom - sphere.center;
    float a = glm::dot(rayDir, rayDir);
    float b = 2 * glm::dot(v, rayDir);
    float c = glm::dot(v, v) - sphere.radius * sphere.radius;
    float d = b * b - 4 * a * c;
    if (d < 0) {
        return Hit(false);
    } else if (fabsf(d) < EPS) {
        float t = -b / 2 * a;
        if (t > EPS) {
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return Hit(true, t, hitPt, hitPt - sphere.center, mtl.diffusiveColor);
        } else {
            return Hit(false);
        }
    } else {
        float sqrtD = sqrtf(d);
        float t1 = (-b + sqrtD) / (2 * a);
        float t2 = (-b - sqrtD) / (2 * a);
        if (t1 > EPS && t2 > EPS) {
            float t = t1 < t2 ? t1 : t2;
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return Hit(true, t, hitPt, hitPt - sphere.center, mtl.diffusiveColor);
        } else if (t1 > EPS) {
            float t = t1;
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return Hit(true, t, hitPt, hitPt - sphere.center, mtl.diffusiveColor);
        } else if (t2 > EPS) {
            float t = t2;
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return Hit(true, t, hitPt, hitPt - sphere.center, mtl.diffusiveColor);
        } else {
            return Hit(false);
        }
    }
}

void
loadScene(
        Scene &outScene,
        const std::string &pathToScene
) {
    std::ifstream is(pathToScene);
    Json inputJson;
    is >> inputJson;

    Json world = inputJson["world"];
    if (world != nullptr) {
        outScene.worldAmbientColor = glm::vec3(world["ambientColor"][0], world["ambientColor"][1],
                                               world["ambientColor"][2]);
        outScene.worldHorizonColor = glm::vec3(world["horizonColor"][0], world["horizonColor"][1],
                                               world["horizonColor"][2]);
        outScene.worldAmbientFactor = world["ambientFactor"];
    } else {
        outScene.worldAmbientColor = glm::vec3(0.0, 0.0, 0.0);
        outScene.worldHorizonColor = glm::vec3(0.3, 0.3, 0.3);
        outScene.worldAmbientFactor = 0.0;
    }

    for (Json &i : inputJson["materials"]) {
        std::shared_ptr<Material> material(new Material());
        material->diffusiveIntensity = i["diffusiveFactor"];
        material->diffusiveColor = glm::vec3(i["diffusiveColor"][0], i["diffusiveColor"][1], i["diffusiveColor"][2]);
        if (i["imagePath"] != nullptr) {
            material->texImage = TexImage::createImage(i["imagePath"]);
            material->texImage->setScaleX(i["scaleX"]);
            material->texImage->setScaleY(i["scaleY"]);
        }
        outScene.materials.push_back(material);
    }

    std::vector<glm::vec3> vertices;
    for (Json &i : inputJson["vertices"]) {
        float x = i[0];
        float y = i[1];
        float z = i[2];
        vertices.push_back(glm::vec3(x, y, z));
    }

    for (Json &i : inputJson["faces"]) {
        auto p1 = vertices[i["vertices"][0]];
        auto p2 = vertices[i["vertices"][1]];
        auto p3 = vertices[i["vertices"][2]];
        glm::vec2 uv1(0.0f, 0.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(0.0f, 0.0f);
        if (i["uv"] != nullptr) {
            uv1[0] = i["uv"][0][0];
            uv1[1] = i["uv"][0][1];
            uv2[0] = i["uv"][1][0];
            uv2[1] = i["uv"][1][1];
            uv3[0] = i["uv"][2][0];
            uv3[1] = i["uv"][2][1];
        }
        std::shared_ptr<Material> material;
        if (i["material"] != nullptr) {
            material = outScene.materials[i["material"]];
            if (material->texImage.get() != nullptr) {
                uv1[0] *= (material->texImage->getWidth() * material->texImage->getScaleX());
                uv1[1] *= (material->texImage->getHeight() * material->texImage->getScaleY());
                uv2[0] *= (material->texImage->getWidth() * material->texImage->getScaleX());
                uv2[1] *= (material->texImage->getHeight() * material->texImage->getScaleY());
                uv3[0] *= (material->texImage->getWidth() * material->texImage->getScaleX());
                uv3[1] *= (material->texImage->getHeight() * material->texImage->getScaleY());
            }
        } else {
            material.reset(new Material());
        }
        std::shared_ptr<Triangle> triangle(new Triangle(p1, p2, p3, uv1, uv2, uv3, material));
        outScene.triangles.push_back(triangle);
    }

    for (Json &i : inputJson["spheres"]) {
        int x = i["center"][0];
        int y = i["center"][1];
        int z = i["center"][2];
        float radius = i["radius"];
        std::shared_ptr<Material> material;
        if (i["material"] != nullptr) {
            material = outScene.materials[i["material"]];
        } else {
            material.reset(new Material());
        }
        std::shared_ptr<Sphere> sphere(new Sphere(glm::vec3(x, y, z), radius, material));
        outScene.spheres.push_back(sphere);
    }

    for (Json &i : inputJson["lamps"]) {
        int x = i["pos"][0];
        int y = i["pos"][1];
        int z = i["pos"][2];
        float intensity = i["intensity"];
        std::shared_ptr<Lamp> lamp(new Lamp(glm::vec3(x, y, z), intensity));
        outScene.lamps.push_back(lamp);
    }

    auto translate = inputJson["translate"];
    auto transform = inputJson["transform"];
    glm::vec3 pos(static_cast<float>(translate[0]), static_cast<float>(translate[1]),
                  static_cast<float>(translate[2]));
    glm::mat3 mat(
            static_cast<float>(transform[0][0]), static_cast<float>(transform[0][1]),
            static_cast<float>(transform[0][2]),
            static_cast<float>(transform[1][0]), static_cast<float>(transform[1][1]),
            static_cast<float>(transform[1][2]),
            static_cast<float>(transform[2][0]), static_cast<float>(transform[2][1]),
            static_cast<float>(transform[2][2])
    );
    outScene.camPos = pos;
    outScene.camMat = mat;
}

void
renderScene(
        ImageBitmap &outImg,
        const Scene &scene
) {
    auto width = outImg.getWidth();
    auto height = outImg.getHeight();
    float camHeight = 0.5;
    float camWidth = static_cast<float>(width) * (camHeight / static_cast<float>(height));
    float camDist = 1.0;
    float dh = camHeight / static_cast<float>(height);
    float dw = camWidth / static_cast<float>(width);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            float rayX = -(camWidth / 2) + j * dw;
            float rayY = -(camHeight / 2) + i * dh;
            glm::vec3 rayCamDir(rayX, rayY, -camDist);
            glm::vec3 rayDx(dw / 2.0f, 0.0f, 0.0f);
            glm::vec3 rayDy(0.0f, dh / 2.0f, 0.0f);
            glm::vec3 rayWorldDir = rayCamDir * scene.camMat;
            glm::vec3 rayWorldDx = rayDx * scene.camMat;
            glm::vec3 rayWorldDy = rayDy * scene.camMat;

            bool log = false;

            auto traceColor = traceRay(scene, scene.camPos, rayWorldDir, log);
            traceColor += traceRay(scene, scene.camPos, rayWorldDir + rayWorldDx, log);
            traceColor += traceRay(scene, scene.camPos, rayWorldDir + rayWorldDy, log);
            traceColor += traceRay(scene, scene.camPos, rayWorldDir + rayWorldDx + rayWorldDy, log);
            traceColor /= 4.0f;
            traceColor *= 255.f;
            uint8_t r = static_cast<uint8_t>(std::min(255.0f, traceColor.x));
            uint8_t g = static_cast<uint8_t>(std::min(255.0f, traceColor.y));
            uint8_t b = static_cast<uint8_t>(std::min(255.0f, traceColor.z));
            outImg.setPixel(j, i, r, g, b);
        }
    }
}

glm::vec3
traceRay(
        const Scene &scene,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir,
        bool log
) {
    Hit closestHit(false);
    std::shared_ptr<Material> closestMaterial;

    for (auto &obj : scene.triangles) {
        auto hit = computeHit(*obj, rayFrom, rayDir, *obj->material);
        if (hit.isHit && hit.t > 0 && (!closestHit.isHit || hit.t < closestHit.t)) {
            closestHit = hit;
            closestMaterial = obj->material;
        }
    }

    for (auto &obj : scene.spheres) {
        auto hit = computeHit(*obj, rayFrom, rayDir, *obj->material);
        if (hit.isHit && hit.t > 0 && (!closestHit.isHit || hit.t < closestHit.t)) {
            closestHit = hit;
            closestMaterial = obj->material;
        }
    }

    if (!closestHit.isHit) {
        return scene.worldHorizonColor;
    }

    glm::vec3 hitPt = closestHit.point;
    glm::vec3 hitNorm = glm::normalize(closestHit.norm);
    glm::vec3 hitColor = closestHit.color;

    glm::vec3 retColor = scene.worldAmbientColor;
    retColor += computeDiffusiveLight(scene, hitPt, hitNorm, rayDir, closestMaterial->diffusiveIntensity, hitColor);
    retColor += computeAmbientOcclusion(scene, hitPt, hitNorm, rayDir, hitColor);

    return retColor;
}

glm::vec3
computeDiffusiveLight(
        const Scene &scene,
        const glm::vec3 &pt,
        const glm::vec3 &norm,
        const glm::vec3 &rayDir,
        const float k,
        const glm::vec3 &color
) {
    glm::vec3 retColor(0.0f, 0.0f, 0.0f);
    for (auto &lamp : scene.lamps) {
        bool shaded = false;
        glm::vec3 toLamp = lamp->pos - pt;

        float dotWithLamp = glm::dot(toLamp, norm);
        float dotWithDir = glm::dot(rayDir, norm);
        if ((dotWithDir < 0.0 && dotWithLamp < 0.0) || (dotWithDir > 0.0 && dotWithLamp > 0.0)) {
            shaded = true;
        }

        Hit hit;
        for (auto &obj : scene.triangles) {
            if (shaded) {
                break;
            }

            hit = computeHit(*obj, pt, toLamp, *obj->material);
            if (hit.isHit) {
                shaded = true;
            }
        }

        for (auto &obj : scene.spheres) {
            if (shaded) {
                break;
            }

            hit = computeHit(*obj, pt, toLamp, *obj->material);
            if (hit.isHit) {
                shaded = true;
            }
        }

        if (!shaded) {
            toLamp = glm::normalize(toLamp);
            float dot = fabsf(glm::dot(norm, toLamp));
            retColor += color * k * lamp->intensity * dot;
        }
    }

    return retColor;
}

glm::vec3
computeAmbientOcclusion(
        const Scene &scene,
        const glm::vec3 &pt,
        const glm::vec3 &norm,
        const glm::vec3 &rayDir,
        const glm::vec3 color
) {
    glm::vec3 retColor(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < AO_RAYS_COUNT; i++) {
        glm::vec3 realNorm = glm::dot(rayDir, norm) < 0.0f ? norm : -norm;
        glm::vec3 randomRay;
        do {
            float randX = fabsf(2.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 1.0f;
            float randY = fabsf(2.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 1.0f;
            float randZ = fabsf(2.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 1.0f;
            randomRay = glm::vec3(randX, randY, randZ);
        } while (glm::dot(realNorm, randomRay) < EPS);

        bool shaded = false;

        Hit hit;
        for (auto &j : scene.triangles) {
            if (shaded) {
                break;
            }

            hit = computeHit(*j, pt, randomRay, *j->material);
            if (hit.isHit) {
                shaded = true;
            }
        }

        for (auto &j : scene.spheres) {
            if (shaded) {
                break;
            }

            hit = computeHit(*j, pt, randomRay, *j->material);
            if (hit.isHit) {
                shaded = true;
            }
        }

        if (!shaded) {
            retColor += color * scene.worldAmbientFactor;
        }
    }

    return retColor / static_cast<float>(AO_RAYS_COUNT);
}

