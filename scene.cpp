//
// Created by vlad on 4/15/17.
//

#include <fstream>
#include <thread>
#include "scene.h"
#include "lib/json.h"

#define AO_RAYS_COUNT (30)
#define MAX_REFLECTION_DEPTH (2)


using Json = nlohmann::json;

void
renderScene(
        image_bitmap &outImage,
        const Scene &scene,
        const std::shared_ptr<OpenClExecutor> clExecutor
) {
    renderScene(outImage, scene, clExecutor, outImage.getWidth(), outImage.getHeight(), 0, 0);
}

void
renderScene(
        image_bitmap &outImg,
        const Scene &scene,
        const std::shared_ptr<OpenClExecutor> clExecutor,
        int fullWidth,
        int fullHeight,
        int x,
        int y
) {

    auto width = fullWidth;
    auto height = fullHeight;
    float camHeight = 0.5;
    float camWidth = static_cast<float>(width) * (camHeight / static_cast<float>(height));
    float camDist = 1.0;
    float dh = camHeight / static_cast<float>(height);
    float dw = camWidth / static_cast<float>(width);
    for (int i = 0; i < outImg.getHeight(); i++) {
        for (int j = 0; j < outImg.getWidth(); j++) {
            float rayX = -(camWidth / 2) + (j + x) * dw;
            float rayY = -(camHeight / 2) + (i + y) * dh;
            glm::vec3 rayCamDir(rayX, rayY, -camDist);
            glm::vec3 rayDx(dw / 2.0f, 0.0f, 0.0f);
            glm::vec3 rayDy(0.0f, dh / 2.0f, 0.0f);
            glm::vec3 rayWorldDir = rayCamDir * scene.camMat;
            glm::vec3 rayWorldDx = rayDx * scene.camMat;
            glm::vec3 rayWorldDy = rayDy * scene.camMat;
            auto traceColor = traceRay(scene, clExecutor, scene.camPos, glm::normalize(rayWorldDir), 0);
            traceColor += traceRay(scene, clExecutor, scene.camPos, glm::normalize(rayWorldDir + rayWorldDx), 0);
            traceColor += traceRay(scene, clExecutor, scene.camPos, glm::normalize(rayWorldDir + rayWorldDy), 0);
            traceColor += traceRay(scene, clExecutor, scene.camPos, glm::normalize(rayWorldDir + rayWorldDx + rayWorldDy), 0);
            traceColor /= 4.0f;
            outImg.setPixel(j, i,
                            powf(traceColor.r / 2.2f, 0.3f),
                            powf(traceColor.g / 2.2f, 0.3f),
                            powf(traceColor.b / 2.2f, 0.3f)
            );
        }
    }
}


std::shared_ptr<bool>
renderParallel(
        std::shared_ptr<Scene> scene,
        std::shared_ptr<synchronized_queue<std::tuple<int, int, std::shared_ptr<image_bitmap>>>> outQueue,
        int width,
        int height
) {
    std::shared_ptr<std::vector<std::tuple<int, int, int, int>>> inQueue(
            new std::vector<std::tuple<int, int, int, int>>()
    );
    std::shared_ptr<std::mutex> inQueueMutex(new std::mutex());
    std::shared_ptr<bool> finishedFlag(new bool(false));
    int fullHorzBlocks = width / SUB_BLOCK_WIDTH;
    int fullVertBlocks = width / SUB_BLOCK_WIDTH;
    int lastHorzBlockWidth = width % SUB_BLOCK_WIDTH;
    int lastVertBlockHeight = width % SUB_BLOCK_WIDTH;
    for (int i = 0; i < fullVertBlocks; i++) {
        for (int j = 0; j < fullHorzBlocks; j++) {
            inQueue->push_back(std::make_tuple(
                    j * SUB_BLOCK_WIDTH,
                    i * SUB_BLOCK_HEIGHT,
                    SUB_BLOCK_WIDTH,
                    SUB_BLOCK_HEIGHT
            ));
        }
    }

    if (lastHorzBlockWidth > 0) {
        for (int i = 0; i < fullVertBlocks; i++) {
            inQueue->push_back(std::make_tuple(
                    fullHorzBlocks * SUB_BLOCK_WIDTH,
                    i * SUB_BLOCK_HEIGHT,
                    lastHorzBlockWidth,
                    SUB_BLOCK_HEIGHT
            ));
        }
    }

    if (lastVertBlockHeight > 0) {
        for (int i = 0; i < fullHorzBlocks; i++) {
            inQueue->push_back(std::make_tuple(
                    i * SUB_BLOCK_WIDTH,
                    fullVertBlocks * SUB_BLOCK_HEIGHT,
                    SUB_BLOCK_WIDTH,
                    lastVertBlockHeight
            ));
        }
    }

    if (lastHorzBlockWidth > 0 && lastVertBlockHeight > 0) {
        inQueue->push_back(std::make_tuple(
                fullHorzBlocks * SUB_BLOCK_WIDTH,
                fullVertBlocks * SUB_BLOCK_HEIGHT,
                lastHorzBlockWidth,
                lastVertBlockHeight
        ));
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        std::thread thread(taskProcessor, scene, finishedFlag, outQueue, inQueue, inQueueMutex, width, height);
        thread.detach();
    }

    return finishedFlag;
}


void taskProcessor(
        std::shared_ptr<Scene> scene,
        std::shared_ptr<bool> finishedFlag,
        std::shared_ptr<synchronized_queue<std::tuple<int, int, std::shared_ptr<image_bitmap>>>> outQueue,
        std::shared_ptr<std::vector<std::tuple<int, int, int, int>>> inQueue,
        std::shared_ptr<std::mutex> inQueueMutex,
        int fullWidth,
        int fullHeight
) {
    while (true) {
        int x, y, w, h;
        {
            std::lock_guard<std::mutex> lock(*inQueueMutex);
            if (inQueue->size() == 0) {
                *finishedFlag = true;
                return;
            }

            auto subBlock = (*inQueue)[inQueue->size() - 1];
            x = std::get<0>(subBlock);
            y = std::get<1>(subBlock);
            w = std::get<2>(subBlock);
            h = std::get<3>(subBlock);
            inQueue->pop_back();
        }
        std::shared_ptr<image_bitmap> outImg(new image_bitmap(w, h));
        renderScene(*outImg, *scene, std::shared_ptr<OpenClExecutor>(nullptr), fullWidth, fullHeight, x, y);
        outQueue->push_back(std::make_tuple(x, y, outImg));
    }
}


glm::vec3
traceRay(
        const Scene &scene,
        const std::shared_ptr<OpenClExecutor> clExecutor,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir,
        uint32_t depth
) {
    Hit hit(false);
    if (clExecutor.get() != nullptr) {
        hit = computeClosestHitCl(scene, clExecutor, rayFrom, rayDir);
    } else {
        hit = computeClosestHit(scene, rayFrom, rayDir);
    }

    if (hit.isHit) {
        glm::vec3 retColor =
                scene.worldAmbientColor + hit.color * computeAmbientOcclusion(scene, clExecutor, hit.point, hit.norm, rayDir);
        for (auto &lamp : scene.lamps) {
            bool shaded = false;
            glm::vec3 toLamp = lamp->pos - hit.point;

            float dotWithLamp = glm::dot(toLamp, hit.norm);
            float dotWithDir = glm::dot(rayDir, hit.norm);
            if ((dotWithDir < 0.0 && dotWithLamp < 0.0) || (dotWithDir > 0.0 && dotWithLamp > 0.0)) {
                shaded = true;
            }

            if (!shaded) {
                shaded |= computeAnyHit(scene, hit.point, toLamp);
            }

            if (!shaded) {
                retColor += hit.color * hit.mtl->diffusiveFactor * computeDiffusiveLight(*lamp, toLamp, hit.norm);
                retColor += hit.color * hit.mtl->specularFactor *
                            computePhongLight(*lamp, toLamp, hit.norm, rayDir, hit.mtl->specularHardness);
            }
        }
        if (hit.mtl->reflectionFactor > EPS && depth < MAX_REFLECTION_DEPTH) {
            retColor += traceRay(scene, clExecutor, hit.point, rayDir - 2.0f * hit.norm * glm::dot(rayDir, hit.norm), depth + 1) *
                        hit.mtl->reflectionFactor;
        }
        return retColor;
    } else {
        return scene.worldHorizonColor;
    }
}


Hit
computeClosestHit(
        const Scene &scene,
        const std::shared_ptr<OpenClExecutor> clExecutor,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
) {
    TriangleHit closestTriangleHit(false);
    std::shared_ptr<Triangle> closestTriangle;
    if (clExecutor.get() == nullptr) {
        for (auto &obj : scene.triangles) {
            auto hit = computeTriangleHit(*obj, rayFrom, rayDir);
            if (hit.isHit && hit.t > 0 && (!closestTriangleHit.isHit || hit.t < closestTriangleHit.t)) {
                closestTriangleHit = hit;
                closestTriangle = obj;
            }
        }
    } else {

    }

    SphereHit closestSphereHit(false);
    if (clExecutor.get() == nullptr) {
        std::shared_ptr<Sphere> closestSphere;
        for (auto &obj : scene.spheres) {
            auto hit = computeSphereHit(*obj, rayFrom, rayDir);
            if (hit.isHit && hit.t > 0 && (!closestSphereHit.isHit || hit.t < closestSphereHit.t)) {
                closestSphereHit = hit;
                closestSphere = obj;
            }
        }
    }

    if (closestTriangleHit.isHit && (!closestSphereHit.isHit || closestTriangleHit.t < closestSphereHit.t)) {
        /* Triangle */
        if (closestTriangle->material->textured) {
            glm::vec2 uvCoord = closestTriangle->uvStart
                                + closestTriangle->uvU * closestTriangleHit.u
                                + closestTriangle->uvV * closestTriangleHit.v;
            glm::vec4 rgbaColor = closestTriangle->material->texImage->get((unsigned int) uvCoord.x,
                                                                           (unsigned int) uvCoord.y);
            glm::vec3 rgbColor = rgbaColor.a * glm::vec3(rgbaColor.r, rgbaColor.g, rgbaColor.b)
                                 + (1 - rgbaColor.a) * closestTriangle->material->color;
            return Hit(
                    true,
                    closestTriangle->material,
                    closestTriangleHit.point,
                    closestTriangleHit.norm,
                    rgbColor,
                    closestTriangleHit.t
            );

        } else {
            return Hit(
                    true,
                    closestTriangle->material,
                    closestTriangleHit.point,
                    closestTriangleHit.norm,
                    closestTriangle->material->color,
                    closestTriangleHit.t
            );
        }
    } else if (closestSphereHit.isHit) {
        /* Sphere */
        return Hit(
                true,
                closestSphere->material,
                closestSphereHit.point,
                closestSphereHit.norm,
                closestSphere->material->color,
                closestSphereHit.t
        );
    } else {
        /* None */
        return Hit(false);
    }
}


bool
computeAnyHit(
        const Scene &scene,
        const std::shared_ptr<OpenClExecutor> clExecutor,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
) {
    for (auto &obj : scene.triangles) {
        auto hit = computeTriangleHit(*obj, rayFrom, rayDir);
        if (hit.isHit) {
            return true;
        }
    }

    for (auto &obj : scene.spheres) {
        auto hit = computeSphereHit(*obj, rayFrom, rayDir);
        if (hit.isHit) {
            return true;
        }
    }

    return false;
}


TriangleHit
computeTriangleHit(
        const Triangle &triangle,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
) {
    auto q = rayFrom - triangle.p;
    auto det = glm::dot(rayDir, glm::cross(triangle.e2, triangle.e1));

    if (fabsf(det) < EPS) {
        return TriangleHit(false);
    }

    auto dett = glm::dot(q, glm::cross(triangle.e1, triangle.e2));
    auto t = dett / det;
    if (t < EPS) {
        return TriangleHit(false);
    }

    auto detu = glm::dot(rayDir, glm::cross(triangle.e2, q));
    auto detv = glm::dot(rayDir, glm::cross(q, triangle.e1));
    auto u = detu / det;
    auto v = detv / det;
    if (u < 0.0 || v < 0.0 || v + u > 1.0) {
        return TriangleHit(false);
    }

    auto hitPt = rayFrom + t * rayDir;
    return TriangleHit(true, t, u, v, hitPt, triangle.norm);
}


SphereHit
computeSphereHit(
        const Sphere &sphere,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
) {
    glm::vec3 v = rayFrom - sphere.center;
    float a = glm::dot(rayDir, rayDir);
    float b = 2 * glm::dot(v, rayDir);
    float c = glm::dot(v, v) - sphere.radius * sphere.radius;
    float d = b * b - 4 * a * c;
    if (d < 0) {
        return SphereHit(false);
    } else if (fabsf(d) < EPS) {
        float t = -b / 2 * a;
        if (t > EPS) {
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return SphereHit(true, t, hitPt, (hitPt - sphere.center) / sphere.radius);
        } else {
            return SphereHit(false);
        }
    } else {
        float sqrtD = sqrtf(d);
        float t1 = (-b + sqrtD) / (2 * a);
        float t2 = (-b - sqrtD) / (2 * a);
        if (t1 > EPS && t2 > EPS) {
            float t = t1 < t2 ? t1 : t2;
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return SphereHit(true, t, hitPt, (hitPt - sphere.center) / sphere.radius);
        } else if (t1 > EPS) {
            float t = t1;
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return SphereHit(true, t, hitPt, (hitPt - sphere.center) / sphere.radius);
        } else if (t2 > EPS) {
            float t = t2;
            glm::vec3 hitPt = rayFrom + t * rayDir;
            return SphereHit(true, t, hitPt, (hitPt - sphere.center) / sphere.radius);
        } else {
            return SphereHit(false);
        }
    }
}


float
computeDiffusiveLight(
        const Lamp &lamp,
        const glm::vec3 &toLamp,
        const glm::vec3 &norm
) {
    float dot = fabsf(glm::dot(norm, toLamp));
    float sqrLength = glm::dot(toLamp, toLamp);
    return lamp.intensity * lamp.distance * dot / sqrLength;
}


float
computePhongLight(
        const Lamp &lamp,
        const glm::vec3 &toLamp,
        const glm::vec3 &norm,
        const glm::vec3 &rayDir,
        float hardness
) {
    auto toLampReflected = glm::normalize(toLamp - 2.0f * norm * glm::dot(toLamp, norm));
    auto dot = glm::dot(toLampReflected, rayDir);
    if (dot > 0.0f) {
        return (hardness * lamp.distance / glm::dot(toLamp, toLamp)) * powf(dot, hardness);
    } else {
        return 0.0f;
    }
}


float
computeAmbientOcclusion(
        const Scene &scene,
        const std::shared_ptr<OpenClExecutor> clExecutor,
        const glm::vec3 &pt,
        const glm::vec3 &norm,
        const glm::vec3 &rayDir
) {
    float retVal = 0.0f;
    for (int i = 0; i < AO_RAYS_COUNT; i++) {
        glm::vec3 realNorm = glm::dot(rayDir, norm) < 0.0f ? norm : -norm;
        glm::vec3 randomRay = generateRandomRayInHalfSphere(realNorm);

        bool inShade;
        if (clExecutor.get() != nullptr) {
            inShade = computeAnyHitCl(scene, clExecutor, pt, randomRay);
        } else {
            inShade = computeAnyHit(scene, pt, randomRay);
        }

        if (!inShade) {
            retVal += scene.worldAmbientFactor;
        }
    }
    return retVal / static_cast<float>(AO_RAYS_COUNT);
}


glm::vec3 generateRandomRayInHalfSphere(const glm::vec3 &norm) {
    glm::vec3 randomRay;
    float randX = fabsf(2.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 1.0f;
    float randY = fabsf(2.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 1.0f;
    float randZ = fabsf(2.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 1.0f;
    randomRay = glm::vec3(randX, randY, randZ);
    return glm::dot(randomRay, norm) < EPS ? -randomRay : randomRay;
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

    for (Json &i : inputJson["mSpheres"]) {
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

bool computeAnyHitCl(const Scene &scene, const std::shared_ptr<OpenClExecutor> clExecutor, const glm::vec3 &rayFrom,
                     const glm::vec3 &rayDir) {
    return false;
}

Hit computeClosestHitCl(const Scene &scene, const std::shared_ptr<OpenClExecutor> clExecutor, const glm::vec3 &rayFrom,
                        const glm::vec3 &rayDir) {
    return Hit(false);
}
