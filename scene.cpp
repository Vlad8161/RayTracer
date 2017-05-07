//
// Created by vlad on 4/15/17.
//

#include <fstream>
#include <thread>
#include "scene.h"
#include "lib/json.h"
#include "opencl_executor.h"

using Json = nlohmann::json;



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
        material->color = glm::vec3(i["diffusiveColor"][0], i["diffusiveColor"][1], i["diffusiveColor"][2]);
        material->diffusiveFactor = i["diffusiveFactor"];
        material->specularFactor = i["specularFactor"];
        material->specularHardness = i["specularHardness"];
        material->reflectionFactor = i["reflectionFactor"];
        if (i["imagePath"] != nullptr) {
            material->texImage = tex_image::createImage(i["imagePath"]);
            material->texImage->setScaleX(i["scaleX"]);
            material->texImage->setScaleY(i["scaleY"]);
            material->textured = true;
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
        auto p = vertices[i["vertices"][0]];
        auto e1 = vertices[i["vertices"][1]] - p;
        auto e2 = vertices[i["vertices"][2]] - p;
        glm::vec2 uvStart(0.0f, 0.0f);
        glm::vec2 uvU(0.0f, 0.0f);
        glm::vec2 uvV(0.0f, 0.0f);
        if (i["uv"] != nullptr) {
            uvStart[0] = i["uv"][0][0];
            uvStart[1] = i["uv"][0][1];
            uvU[0] = float(i["uv"][1][0]) - uvStart[0];
            uvU[1] = float(i["uv"][1][1]) - uvStart[1];
            uvV[0] = float(i["uv"][2][0]) - uvStart[0];
            uvV[1] = float(i["uv"][2][1]) - uvStart[1];
        }
        std::shared_ptr<Material> material;
        if (i["material"] != nullptr) {
            material = outScene.materials[i["material"]];
            if (material->texImage.get() != nullptr) {
                uvStart[0] *= (material->texImage->getWidth() * material->texImage->getScaleX());
                uvStart[1] *= (material->texImage->getHeight() * material->texImage->getScaleY());
                uvU[0] *= (material->texImage->getWidth() * material->texImage->getScaleX());
                uvU[1] *= (material->texImage->getHeight() * material->texImage->getScaleY());
                uvV[0] *= (material->texImage->getWidth() * material->texImage->getScaleX());
                uvV[1] *= (material->texImage->getHeight() * material->texImage->getScaleY());
            }
        } else {
            material.reset(new Material());
        }
        std::shared_ptr<Triangle> triangle(
                new Triangle(p, e1, e2, uvStart, uvU, uvV, glm::normalize(glm::cross(e1, e2)), material)
        );
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
        float distance = i["distance"];
        std::shared_ptr<Lamp> lamp(new Lamp(glm::vec3(x, y, z), intensity, distance));
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
