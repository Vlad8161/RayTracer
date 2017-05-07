//
// Created by vlad on 5/6/17.
//

#include "ray_tracer_cl.h"
#include "opencl_executor.h"


void
renderSceneCl(
        image_bitmap &outImg,
        const Scene &scene
) {
    std::shared_ptr<OpenClExecutor> clExecutor(new OpenClExecutor(scene));
    int fullHorzBlocks = outImg.getWidth() / SUB_BLOCK_WIDTH;
    int fullVertBlocks = outImg.getHeight() / SUB_BLOCK_HEIGHT;
    int lastHorzBlockWidth = outImg.getWidth() % SUB_BLOCK_WIDTH;
    int lastVertBlockHeight = outImg.getHeight() % SUB_BLOCK_HEIGHT;
    for (int i = 0; i < fullVertBlocks; i++) {
        for (int j = 0; j < fullHorzBlocks; j++) {
            renderSubBlockCl(outImg, scene, clExecutor, j * SUB_BLOCK_WIDTH, i * SUB_BLOCK_HEIGHT, SUB_BLOCK_WIDTH,
                             SUB_BLOCK_HEIGHT, outImg.getWidth(), outImg.getHeight());
        }
    }

    if (lastHorzBlockWidth > 0) {
        for (int i = 0; i < fullVertBlocks; i++) {
            renderSubBlockCl(
                    outImg, scene, clExecutor,
                    fullHorzBlocks * SUB_BLOCK_WIDTH, i * SUB_BLOCK_HEIGHT, lastHorzBlockWidth, SUB_BLOCK_HEIGHT,
                    outImg.getWidth(), outImg.getHeight()
            );
        }
    }

    if (lastVertBlockHeight > 0) {
        for (int i = 0; i < fullHorzBlocks; i++) {
            renderSubBlockCl(
                    outImg, scene, clExecutor,
                    i * SUB_BLOCK_WIDTH, fullVertBlocks * SUB_BLOCK_HEIGHT, SUB_BLOCK_WIDTH, lastVertBlockHeight,
                    outImg.getWidth(), outImg.getHeight()
            );
        }
    }

    if (lastHorzBlockWidth > 0 && lastVertBlockHeight > 0) {
        renderSubBlockCl(
                outImg, scene, clExecutor,
                fullHorzBlocks * SUB_BLOCK_WIDTH,
                fullVertBlocks * SUB_BLOCK_HEIGHT,
                lastHorzBlockWidth,
                lastVertBlockHeight,
                outImg.getWidth(), outImg.getHeight()
        );
    }
}


void
renderSubBlockCl(
        image_bitmap &outImg,
        const Scene &scene,
        std::shared_ptr<OpenClExecutor> clExecutor,
        int x, int y,
        int w, int h,
        int fullWidth, int fullHeight
) {

    auto width = fullWidth;
    auto height = fullHeight;
    float camHeight = 0.5;
    float camWidth = static_cast<float>(width) * (camHeight / static_cast<float>(height));
    float camDist = 1.0;
    float dh = camHeight / static_cast<float>(height);
    float dw = camWidth / static_cast<float>(width);
    std::vector<glm::vec3> tracedColors;
    std::vector<RayData> raysToTrace;
    tracedColors.reserve((unsigned long) (w * h));
    raysToTrace.reserve((unsigned long) (w * h));
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            float rayX = -(camWidth / 2) + (j + x) * dw;
            float rayY = -(camHeight / 2) + (i + y) * dh;
            glm::vec3 rayCamDir = glm::vec3(rayX, rayY, -camDist);
            glm::vec3 rayWorldDir = glm::normalize(rayCamDir * scene.camMat);
            tracedColors.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
            raysToTrace.push_back(RayData(
                    scene.camPos.x, scene.camPos.y, scene.camPos.z,
                    rayWorldDir.x, rayWorldDir.y, rayWorldDir.z
            ));
        }
    }
    traceRaysCl(scene, clExecutor, raysToTrace, tracedColors, 0);
    int k = 0;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            outImg.setPixel(j + x, i + y,
                            powf(tracedColors[k].r / 2.2f, 0.3f),
                            powf(tracedColors[k].g / 2.2f, 0.3f),
                            powf(tracedColors[k].b / 2.2f, 0.3f)
            );
            k++;
        }
    }
}


void
traceRaysCl(
        const Scene &scene,
        std::shared_ptr<OpenClExecutor> clExecutor,
        const std::vector<RayData> &rays,
        std::vector<glm::vec3> &outColors,
        int depth
) {
    std::vector<Hit> hits;
    computeClosestHitsCl(scene, clExecutor, rays, hits);

    for (int i = 0; i < hits.size(); i++) {
        if (!hits[i].isHit) {
            outColors[i] += scene.worldHorizonColor;
        }
    }

    std::vector<char> shaded;
    std::vector<size_t> indexes;
    std::vector<RayData> raysToHit;
    shaded.reserve(rays.size());
    indexes.reserve(rays.size());
    raysToHit.reserve(rays.size());
    for (auto &lamp : scene.lamps) {
        shaded.clear();
        indexes.clear();
        raysToHit.clear();
        for (size_t i = 0; i < hits.size(); i++) {
            if (hits[i].isHit) {
                glm::vec3 toLamp = lamp->pos - hits[i].point;
                float dotWithLamp = glm::dot(toLamp, hits[i].norm);
                float dotWithDir =
                        rays[i].d_x * hits[i].norm.x +
                        rays[i].d_y * hits[i].norm.y +
                        rays[i].d_z * hits[i].norm.z;
                if ((dotWithDir < 0.0 && dotWithLamp >= 0.0) || (dotWithDir > 0.0 && dotWithLamp <= 0.0)) {
                    indexes.push_back(i);
                    shaded.push_back(false);
                    raysToHit.push_back(RayData(
                            hits[i].point.x, hits[i].point.y, hits[i].point.z,
                            toLamp.x, toLamp.y, toLamp.z
                    ));
                }
            }
        }
        computeAnyHitsCl(scene, clExecutor, raysToHit, shaded.data());
        for (size_t i = 0; i < raysToHit.size(); i++) {
            if (!shaded[i]) {
                size_t idx = indexes[i];
                glm::vec3 toLamp(raysToHit[i].d_x, raysToHit[i].d_y, raysToHit[i].d_z);

                /* diffusive */
                float dot = fabsf(glm::dot(hits[idx].norm, toLamp));
                float sqrLength = glm::dot(toLamp, toLamp);
                outColors[idx] +=
                        hits[idx].color * hits[idx].mtl->diffusiveFactor * lamp->intensity * lamp->distance * dot /
                        sqrLength;

                /* specular */
                glm::vec3 rayDir(rays[idx].d_x, rays[idx].d_y, rays[idx].d_z);
                auto toLampReflected = glm::normalize(
                        toLamp - 2.0f * hits[idx].norm * glm::dot(toLamp, hits[idx].norm));
                dot = glm::dot(toLampReflected, rayDir);
                auto specLight = std::max(0.0f, (hits[idx].mtl->specularHardness * lamp->distance /
                                                 glm::dot(toLamp, toLamp)) *
                                                powf(dot, hits[idx].mtl->specularHardness));
                outColors[idx] += hits[idx].color * hits[idx].mtl->specularFactor * specLight;
            }
        }
    }
}


void
computeClosestHitsCl(
        const Scene &scene,
        std::shared_ptr<OpenClExecutor> clExecutor,
        const std::vector<RayData> &rays,
        std::vector<Hit> &hits
) {
    std::vector<std::tuple<TriangleHit, size_t>> tHits;
    clExecutor->computeClosestHitTriangle(reinterpret_cast<const cl_float *>(rays.data()), (cl_uint) rays.size(),
                                          tHits);

    std::vector<std::tuple<SphereHit, size_t>> sHits;
    clExecutor->computeClosestHitSphere(reinterpret_cast<const cl_float *>(rays.data()), (cl_uint) rays.size(), sHits);

    hits.clear();
    hits.resize(rays.size(), Hit(false));
    for (int i = 0; i < rays.size(); i++) {
        bool th = std::get<0>(tHits[i]).isHit;
        bool sh = std::get<0>(sHits[i]).isHit;
        float tt = std::get<0>(tHits[i]).t;
        float st = std::get<0>(sHits[i]).t;
        bool sphere;
        bool triangle;

        if (th) {
            sphere = sh && st < tt;
            triangle = !sphere;
        } else {
            sphere = sh;
            triangle = false;
        }

        if (triangle) {
            auto &tr = scene.triangles[std::get<1>(tHits[i])];
            auto &trh = std::get<0>(tHits[i]);
            glm::vec3 rgbColor;
            if (tr->material->textured) {
                glm::vec2 uvCoord = tr->uvStart + tr->uvU * trh.u + tr->uvV * trh.v;
                glm::vec4 rgbaColor = tr->material->texImage->get((unsigned int) uvCoord.x, (unsigned int) uvCoord.y);
                rgbColor = rgbaColor.a * glm::vec3(rgbaColor.r, rgbaColor.g, rgbaColor.b)
                                     + (1 - rgbaColor.a) * tr->material->color;
            } else {
                rgbColor = tr->material->color;
            }
            hits[i].isHit = true;
            hits[i].mtl = tr->material;
            hits[i].norm = trh.norm;
            hits[i].point = trh.point;
            hits[i].t = trh.t;
            hits[i].color = rgbColor;
        } else if (sphere) {
            auto &sph = scene.spheres[std::get<1>(sHits[i])];
            auto &sphh = std::get<0>(sHits[i]);
            hits[i].isHit = true;
            hits[i].mtl = sph->material;
            hits[i].norm = sphh.norm;
            hits[i].point = sphh.point;
            hits[i].t = sphh.t;
            hits[i].color = sph->material->color;
        }
    }
}


void
computeAnyHitsCl(
        const Scene &scene,
        std::shared_ptr<OpenClExecutor> clExecutor,
        const std::vector<RayData> &rays,
        char *hits
) {
    std::vector<cl_char> bufHits(rays.size(), false);
    clExecutor->computeAnyHitTriangle(reinterpret_cast<const cl_float *>(rays.data()), (cl_uint) rays.size(), bufHits.data());
    for (size_t i = 0; i < rays.size(); i++) {
        hits[i] = hits[i] || bufHits[i];
    }

    clExecutor->computeAnyHitSphere(reinterpret_cast<const cl_float *>(rays.data()), (cl_uint) rays.size(), bufHits.data());
    for (size_t i = 0; i < rays.size(); i++) {
        hits[i] = hits[i] || bufHits[i];
    }
}



