//
// Created by vlad on 4/15/17.
//

#include <fstream>
#include "Scene.h"
#include "json.h"

using Json = nlohmann::json;

std::tuple<bool, float, Hit>
computeHit(
        const Triangle &triangle,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
);

std::tuple<bool, float, Hit>
computeHit(
        const Sphere &sphere,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
);

std::array<float, 3> traceRay(const Scene &scene, const glm::vec3 &rayFrom, const glm::vec3 &rayDir);



std::tuple<bool, float, Hit>
computeHit(const Triangle &triangle, const glm::vec3 &rayFrom, const glm::vec3 &rayDir) {
    auto norm = glm::cross(triangle.p1 - triangle.p2, triangle.p2 - triangle.p3);
    auto normRayDirDot = glm::dot(norm, rayDir);

    if (fabsf(normRayDirDot) < EPS) {
        return std::make_tuple(false, 0.0, Hit());
    }

    auto t = -(glm::dot(norm, rayFrom) - glm::dot(norm, triangle.p1)) / glm::dot(norm, rayDir);
    if (t < EPS) {
        return std::make_tuple(false, 0.0, Hit());
    }

    auto intersectPt = rayFrom + t * rayDir;
    auto a = glm::distance(triangle.p1, triangle.p2);
    auto b = glm::distance(triangle.p2, triangle.p3);
    auto c = glm::distance(triangle.p3, triangle.p1);
    auto aa = glm::distance(triangle.p1, intersectPt);
    auto bb = glm::distance(triangle.p1, intersectPt);
    auto cc = glm::distance(triangle.p1, intersectPt);
    auto p = (a + b + c) / 2;
    auto sqrMain = sqrtf(p * (p - a) * (p - b) * (p - c));
    p = (a + aa + bb) / 2;
    auto sqr1 = sqrtf(p * (p - a) * (p - aa) * (p - bb));
    p = (b + bb + cc) / 2;
    auto sqr2 = sqrtf(p * (p - b) * (p - bb) * (p - cc));
    p = (c + aa + cc) / 2;
    auto sqr3 = sqrtf(p * (p - c) * (p - aa) * (p - cc));
    if (fabsf(sqrMain - (sqr1 + sqr2 + sqr3)) > EPS) {
        return std::make_tuple(false, 0.0, Hit());
    }

    return std::make_tuple(true, t, Hit(intersectPt, norm));
}

std::tuple<bool, float, Hit>
computeHit(const Sphere &sphere, const glm::vec3 &rayFrom, const glm::vec3 &rayDir) {
    return std::tuple<bool, float, Hit>(false, 0.0, Hit());
}

void loadScene(Scene &outScene, const std::string &pathToScene) {
    std::ifstream is(pathToScene);
    Json inputJson;
    is >> inputJson;

    for (Json &i : inputJson["materials"]) {
        std::shared_ptr<Material> material(new Material());
        material->diffusiveIntensity = i["diffInt"];
        material->diffusiveColorRed = i["diffRed"];
        material->diffusiveColorGreen = i["diffGreen"];
        material->diffusiveColorBlue = i["diffBlue"];
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
        std::shared_ptr<Material> material = outScene.materials[i["material"]];
        std::shared_ptr<Triangle> triangle(new Triangle(p1, p2, p3, material));
        outScene.triangles.push_back(triangle);
    }

    for (Json &i : inputJson["spheres"]) {
        int x = i["center"][0];
        int y = i["center"][1];
        int z = i["center"][2];
        float radius = i["radius"];
        std::shared_ptr<Material> material = outScene.materials[i["material"]];
        std::shared_ptr<Sphere> sphere(new Sphere(glm::vec3(x, y, z), radius, material));
        outScene.spheres.push_back(sphere);
    }

    for (Json &i : inputJson["lamps"]) {
        int x = i["pos"][0];
        int y = i["pos"][1];
        int z = i["pos"][2];
        float intensity = i["intensity"];
        std::shared_ptr<Lamp> lamp(new Lamp(glm::vec3(x, y, z), intensity));
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

void renderScene(ImageBitmap &outImg, const Scene &scene) {
    auto width = outImg.getWidth();
    auto height = outImg.getHeight();
    float camHeight = 1.0;
    float camWidth = static_cast<float>(width) * (camHeight / static_cast<float>(height));
    float camDist = 1.0;
    float dh = camHeight / static_cast<float>(height);
    float dw = camWidth / static_cast<float>(width);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            float rayX = -(camWidth / 2) + j * dw;
            float rayY = -(camHeight / 2) + i * dh;
            glm::vec3 rayCamDir(rayX, rayY, -camDist);
            glm::vec3 rayWorldDir = rayCamDir * scene.camMat;
            auto traceColor = traceRay(scene, scene.camPos, rayWorldDir);
            uint8_t r = static_cast<uint8_t>(std::min(255.0f, std::get<0>(traceColor)));
            uint8_t g = static_cast<uint8_t>(std::min(255.0f, std::get<1>(traceColor)));
            uint8_t b = static_cast<uint8_t>(std::min(255.0f, std::get<2>(traceColor)));
            outImg.setPixel(j, i, r, g, b);
        }
    }
}

std::array<float, 3> traceRay(const Scene &scene, const glm::vec3 &rayFrom, const glm::vec3 &rayDir) {
    std::tuple<bool, float, Hit> closestHit(false, 0.0, Hit());
    std::shared_ptr<Triangle> closestTriangle;
    for (auto &obj : scene.triangles) {
        auto hit = computeHit(*obj, rayFrom, rayDir);
        if (std::get<0>(hit) && std::get<1>(hit) > 0 &&
            (!std::get<0>(closestHit) || std::get<1>(hit) < std::get<1>(closestHit))) {
            closestHit = hit;
            closestTriangle = obj;
        }
    }

    if (!std::get<0>(closestHit)) {
        return {150.0, 150.0, 150.0};
    }

    glm::vec3 hitPt = std::get<2>(closestHit).point;
    glm::vec3 hitNorm = glm::normalize(std::get<2>(closestHit).norm);

    float retR = 0.0;
    float retG = 0.0;
    float retB = 0.0;
    for (auto &lamp : scene.lamps) {
        bool inShade = false;

        for (auto &obj : scene.triangles) {
            if (inShade) {
                break;
            }

            if (std::get<0>(computeHit(*obj, hitPt, lamp->pos - hitPt))) {
                inShade = true;
            }
        }

        for (auto &obj : scene.spheres) {
            if (inShade) {
                break;
            }

            if (std::get<0>(computeHit(*obj, hitPt, lamp->pos - hitPt))) {
                inShade = true;
            }
        }

        if (!inShade) {
            glm::vec3 toLamp = glm::normalize(lamp->pos - hitPt);
            float dot = fabsf(glm::dot(hitNorm, toLamp));
            float k = closestTriangle->material->diffusiveIntensity * lamp->intensity * dot;
            retR += closestTriangle->material->diffusiveColorRed * k;
            retG += closestTriangle->material->diffusiveColorGreen * k;
            retB += closestTriangle->material->diffusiveColorBlue * k;
        }
    }

    return {retR, retG, retB};
}
