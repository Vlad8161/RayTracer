//
// Created by vlad on 5/6/17.
//

#ifndef RAY_TRACING_RAY_TRACER_H
#define RAY_TRACING_RAY_TRACER_H

#include "image_bitmap.h"
#include "scene.h"


void
renderScene(
        image_bitmap &outImg,
        const Scene &scene
);


void
renderScene(
        image_bitmap &outImg,
        const Scene &scene,
        int fullWidth,
        int fullHeight,
        int x,
        int y
);


std::shared_ptr<bool>
renderParallel(
        std::shared_ptr<Scene> scene,
        std::shared_ptr<synchronized_queue<std::tuple<int, int, std::shared_ptr<image_bitmap>>>> outQueue,
        int width,
        int height
);


void taskProcessor(
        std::shared_ptr<Scene> scene,
        std::shared_ptr<bool> finishedFlag,
        std::shared_ptr<synchronized_queue<std::tuple<int, int, std::shared_ptr<image_bitmap>>>> outQueue,
        std::shared_ptr<std::vector<std::tuple<int, int, int, int>>> inQueue,
        std::shared_ptr<std::mutex> inQueueMutex,
        int fullWidth,
        int fullHeight
);


glm::vec3
traceRay(
        const Scene &scene,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir,
        uint32_t depth
);


Hit
computeClosestHit(
        const Scene &scene,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
);


bool
computeAnyHit(
        const Scene &scene,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
);


TriangleHit
computeTriangleHit(
        const Triangle &triangle,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
);


SphereHit
computeSphereHit(
        const Sphere &sphere,
        const glm::vec3 &rayFrom,
        const glm::vec3 &rayDir
);


float
computeDiffusiveLight(
        const Lamp &lamp,
        const glm::vec3 &toLamp,
        const glm::vec3 &norm
);


float
computeAmbientOcclusion(
        const Scene &scene,
        const glm::vec3 &pt,
        const glm::vec3 &norm,
        const glm::vec3 &rayDir
);


float
computePhongLight(
        const Lamp &lamp,
        const glm::vec3 &toLamp,
        const glm::vec3 &norm,
        const glm::vec3 &rayDir,
        float hardness
);


glm::vec3
generateRandomRayInHalfSphere(
        const glm::vec3 &norm
);
#endif //RAY_TRACING_RAY_TRACER_H
