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
#define BARYCENTRIC_HIT

#ifdef BARYCENTRIC_HIT
    auto e1 = triangle.p2 - triangle.p1;
    auto e2 = triangle.p3 - triangle.p1;
    auto q = rayFrom - triangle.p1;
    auto det = glm::dot(rayDir, glm::cross(e2, e1));

    if (fabsf(det) < EPS) {
        return std::make_tuple(false, 0.0, Hit());
    }

    auto dett = glm::dot(q, glm::cross(e1, e2));
    auto t = dett / det;
    if (t < EPS) {
        return std::make_tuple(false, 0.0, Hit());
    }

    auto detu = glm::dot(rayDir, glm::cross(e2, q));
    auto detv = glm::dot(rayDir, glm::cross(q, e1));
    auto u = detu / det;
    auto v = detv / det;
    if (u < 0.0 || v < 0.0 || v + u > 1.0) {
        return std::make_tuple(false, 0.0, Hit());
    }

    auto hitPt = rayFrom + t * rayDir;
    return std::make_tuple(true, t, Hit(hitPt, glm::cross(e1, e2)));
#endif

#ifdef SIDE_HIT
    auto norm = glm::cross(triangle.p1 - triangle.p2, triangle.p2 - triangle.p3);
    auto normRayDirDot = glm::dot(norm, rayDir);

    if (fabsf(normRayDirDot) < EPS) {
        return std::make_tuple(false, 0.0, Hit());
    }

    auto t = -(glm::dot(norm, rayFrom) - glm::dot(norm, triangle.p1)) / glm::dot(norm, rayDir);
    if (t < EPS) {
        return std::make_tuple(false, 0.0, Hit());
    }

    auto hitPt = rayFrom + t * rayDir;
    auto a = triangle.p2 - triangle.p1;
    auto b = triangle.p3 - triangle.p2;
    auto c = triangle.p1 - triangle.p3;
    auto ca = glm::cross(hitPt - triangle.p1, a);
    auto cb = glm::cross(hitPt - triangle.p2, b);
    auto cc = glm::cross(hitPt - triangle.p3, c);
    auto dotA = glm::dot(ca, norm);
    auto dotB = glm::dot(cb, norm);
    auto dotC = glm::dot(cc, norm);
    if (dotA > 0.0 && dotB > 0.0 && dotC > 0.0) {
        return std::make_tuple(true, t, Hit(hitPt, norm));
    } else if (dotA < 0.0 && dotB < 0.0 && dotC < 0.0) {
        return std::make_tuple(true, t, Hit(hitPt, norm));
    } else {
        return std::make_tuple(false, 0.0, Hit());
    }
#endif

#ifdef SQUARE_HIT
    auto norm = glm::cross(triangle.p1 - triangle.p2, triangle.p2 - triangle.p3);
    auto normRayDirDot = glm::dot(norm, rayDir);

    if (fabsf(normRayDirDot) < EPS) {
        return std::make_tuple(false, 0.0, Hit());
    }

    auto t = -(glm::dot(norm, rayFrom) - glm::dot(norm, triangle.p1)) / glm::dot(norm, rayDir);
    if (t < EPS) {
        return std::make_tuple(false, 0.0, Hit());
    }

    auto hitPt = rayFrom + t * rayDir;
    auto a = glm::distance(triangle.p1, triangle.p2);
    auto b = glm::distance(triangle.p2, triangle.p3);
    auto c = glm::distance(triangle.p3, triangle.p1);
    auto aa = glm::distance(triangle.p1, hitPt);
    auto bb = glm::distance(triangle.p2, hitPt);
    auto cc = glm::distance(triangle.p3, hitPt);
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
    return std::make_tuple(true, t, Hit(hitPt, norm));
#endif

}

std::tuple<bool, float, Hit>
computeHit(const Sphere &sphere, const glm::vec3 &rayFrom, const glm::vec3 &rayDir) {
    glm::vec3 v = rayFrom - sphere.center;
    float a = glm::dot(rayDir, rayDir);
    float b = 2 * glm::dot(v, rayDir);
    float c = glm::dot(v, v) - sphere.radius * sphere.radius;
    float d = b * b - 4 * a * c;
    if (d < 0) {
        return std::tuple<bool, float, Hit>(false, 0.0, Hit());
    } else if (fabsf(d) < EPS) {
        float t = -b / 2 * a;
        if (t > EPS) {
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return std::tuple<bool, float, Hit>(true, t, Hit(hitPt, hitPt - sphere.center));
        } else {
            return std::tuple<bool, float, Hit>(false, 0.0, Hit());
        }
    } else {
        float sqrtD = sqrtf(d);
        float t1 = -b + sqrtD / 2 * a;
        float t2 = -b - sqrtD / 2 * a;
        if (t1 > 0 && t2 > 0) {
            float t = std::min(t1, t2);
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return std::tuple<bool, float, Hit>(true, t, Hit(hitPt, hitPt - sphere.center));
        } else if (t1 > 0) {
            float t = t1;
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return std::tuple<bool, float, Hit>(true, t, Hit(hitPt, hitPt - sphere.center));
        } else if (t2 > 0) {
            float t = t2;
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return std::tuple<bool, float, Hit>(true, t, Hit(hitPt, hitPt - sphere.center));
        } else {
            return std::tuple<bool, float, Hit>(false, 0.0, Hit());
        }
    }
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

void renderScene(ImageBitmap &outImg, const Scene &scene) {
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
    std::shared_ptr<Material> closestMaterial;

    for (auto &obj : scene.triangles) {
        auto hit = computeHit(*obj, rayFrom, rayDir);
        if (std::get<0>(hit) && std::get<1>(hit) > 0 &&
            (!std::get<0>(closestHit) || std::get<1>(hit) < std::get<1>(closestHit))) {
            closestHit = hit;
            closestMaterial = obj->material;
        }
    }

    for (auto &obj : scene.spheres) {
        auto hit = computeHit(*obj, rayFrom, rayDir);
        if (std::get<0>(hit) && std::get<1>(hit) > 0 &&
            (!std::get<0>(closestHit) || std::get<1>(hit) < std::get<1>(closestHit))) {
            closestHit = hit;
            closestMaterial = obj->material;
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
        bool shaded = false;

        for (auto &obj : scene.triangles) {
            if (shaded) {
                break;
            }

            if (std::get<0>(computeHit(*obj, hitPt, lamp->pos - hitPt))) {
                shaded = true;
            }
        }

        for (auto &obj : scene.spheres) {
            if (shaded) {
                break;
            }

            if (std::get<0>(computeHit(*obj, hitPt, lamp->pos - hitPt))) {
                shaded = true;
            }
        }

        if (!shaded) {
            glm::vec3 toLamp = glm::normalize(lamp->pos - hitPt);
            float dot = fabsf(glm::dot(hitNorm, toLamp));
            float k = closestMaterial->diffusiveIntensity * lamp->intensity * dot;
            retR += closestMaterial->diffusiveColorRed * k;
            retG += closestMaterial->diffusiveColorGreen * k;
            retB += closestMaterial->diffusiveColorBlue * k;
        }
    }

    std::cout << retR << " " << retG << " " << retB << std::endl;
    return {retR * 100, retG * 100, retB * 100};
}
