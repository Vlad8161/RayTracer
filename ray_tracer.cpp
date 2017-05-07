//
// Created by vlad on 5/6/17.
//
#include <thread>
#include "ray_tracer.h"

void
renderScene(
        image_bitmap &outImage,
        const Scene &scene
) {
    renderScene(outImage, scene, outImage.getWidth(), outImage.getHeight(), 0, 0);
}

void
renderScene(
        image_bitmap &outImg,
        const Scene &scene,
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
            auto traceColor = traceRay(scene, scene.camPos, glm::normalize(rayWorldDir), 0);
            traceColor += traceRay(scene, scene.camPos, glm::normalize(rayWorldDir + rayWorldDx), 0);
            traceColor += traceRay(scene, scene.camPos, glm::normalize(rayWorldDir + rayWorldDy), 0);
            traceColor += traceRay(scene, scene.camPos, glm::normalize(rayWorldDir + rayWorldDx + rayWorldDy), 0);
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
    int fullVertBlocks = height / SUB_BLOCK_HEIGHT;
    int lastHorzBlockWidth = width % SUB_BLOCK_WIDTH;
    int lastVertBlockHeight = height % SUB_BLOCK_HEIGHT;
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
        renderScene(*outImg, *scene, fullWidth, fullHeight, x, y);
        outQueue->push_back(std::make_tuple(x, y, outImg));
    }
}


glm::vec3
traceRay(
        const Scene &scene,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir,
        uint32_t depth
) {
    Hit hit = computeClosestHit(scene, rayFrom, rayDir);

    if (hit.isHit) {
        glm::vec3 retColor = scene.worldAmbientColor;
#ifdef ENABLE_AO
        retColor += hit.color * computeAmbientOcclusion(scene, hit.point, hit.norm, rayDir);
#endif
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
#ifdef ENABLE_REFLECTION
        if (hit.mtl->reflectionFactor > EPS && depth < MAX_REFLECTION_DEPTH) {
            retColor += traceRay(scene, hit.point, rayDir - 2.0f * hit.norm * glm::dot(rayDir, hit.norm),
                                 depth + 1) *
                        hit.mtl->reflectionFactor;
        }
#endif
        return retColor;
    } else {
        return scene.worldHorizonColor;
    }
}


Hit
computeClosestHit(
        const Scene &scene,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
) {
    TriangleHit closestTriangleHit(false);
    std::shared_ptr<Triangle> closestTriangle;
    for (auto &obj : scene.triangles) {
        auto hit = computeTriangleHit(*obj, rayFrom, rayDir);
        if (hit.isHit && hit.t > 0 && (!closestTriangleHit.isHit || hit.t < closestTriangleHit.t)) {
            closestTriangleHit = hit;
            closestTriangle = obj;
        }
    }

    SphereHit closestSphereHit(false);
    std::shared_ptr<Sphere> closestSphere;
    for (auto &obj : scene.spheres) {
        auto hit = computeSphereHit(*obj, rayFrom, rayDir);
        if (hit.isHit && hit.t > 0 && (!closestSphereHit.isHit || hit.t < closestSphereHit.t)) {
            closestSphereHit = hit;
            closestSphere = obj;
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
        const glm::vec3 &pt,
        const glm::vec3 &norm,
        const glm::vec3 &rayDir
) {
    float retVal = 0.0f;
    for (int i = 0; i < AO_RAYS_COUNT; i++) {
        glm::vec3 realNorm = glm::dot(rayDir, norm) < 0.0f ? norm : -norm;
        glm::vec3 randomRay = generateRandomRayInHalfSphere(realNorm);

        bool inShade = computeAnyHit(scene, pt, randomRay);

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
