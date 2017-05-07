//
// Created by vlad on 5/6/17.
//

#ifndef RAY_TRACING_RAY_TRACER_CL_H
#define RAY_TRACING_RAY_TRACER_CL_H

#include "image_bitmap.h"
#include "scene.h"

void
renderSceneCl(
        image_bitmap &outImg,
        const Scene &scene
);


void
renderSubBlockCl(
        image_bitmap &outImg,
        const Scene &scene,
        std::shared_ptr<OpenClExecutor> clExecutor,
        int x, int y,
        int w, int h,
        int fullWidth, int fullHeight
);


void
traceRaysCl(
        const Scene& scene,
        std::shared_ptr<OpenClExecutor> clExecutor,
        const std::vector<RayData>& rays,
        std::vector<glm::vec3>& outColors,
        int depth
);


void
computeClosestHitsCl(
        const Scene &scene,
        std::shared_ptr<OpenClExecutor> clExecutor,
        const std::vector<RayData>& rays,
        std::vector<Hit> &hits
);


void
computeAnyHitsCl(
        const Scene &scene,
        std::shared_ptr<OpenClExecutor> clExecutor,
        const std::vector<RayData>& rays,
        char* hits
);


#endif //RAY_TRACING_RAY_TRACER_CL_H
