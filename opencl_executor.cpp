//
// Created by vlad on 5/1/17.
//

#include <fstream>
#include "opencl_executor.h"

OpenClExecutor::OpenClExecutor(const Scene &scene)
        : mContext(0), mCommandQueue(0), mProgram(0)
        , mKrnHitTriangle(0), mTriangles(nullptr), mMemTriangles(0)
        , mKrnHitSphere(0), mSpheres(nullptr), mMemSpheres(0) {
    cl_int err;

    /* Creating context */
    cl_uint platformCount;
    err = clGetPlatformIDs(0, nullptr, &platformCount);
    checkClResult(err, "clGetPlatformIDs count");
    if (platformCount == 0) {
        throw std::runtime_error("platform count == 0");
    }

    cl_platform_id platformIds[platformCount];
    err = clGetPlatformIDs(platformCount, platformIds, nullptr);
    checkClResult(err, "clGetPlatformIds");

    cl_context_properties contextProperties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties) platformIds[0], 0};
    mContext = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU, nullptr, nullptr, &err);
    checkClResult(err, "clCreateContextFromType");

    /* Creating command queue */
    size_t actualSize = 0;
    cl_device_id deviceId;
    err = clGetContextInfo(mContext, CL_CONTEXT_DEVICES, sizeof(deviceId), &deviceId, &actualSize);
    checkClResult(err, "clGetContextInfo (deviceId)");
    if (actualSize == 0) {
        throw std::runtime_error("no devices");
    }

    mCommandQueue = clCreateCommandQueue(mContext, deviceId, 0, &err);
    checkClResult(err, "clCreateCommandQueue");

    /* loading kernels */
    std::ifstream ifstream("cl_kernels.c");
    std::string line;
    std::string progSource;
    while (std::getline(ifstream, line)) {
        progSource += line;
        progSource += "\n";
    }

    cl_uint numProgs = 1;
    size_t progLengthArray[] = {progSource.size()};
    const char *progSourceRaw = progSource.data();
    mProgram = clCreateProgramWithSource(mContext, numProgs, &progSourceRaw, progLengthArray, &err);
    checkClResult(err, "clCreateProgramWithSource");

    err = clBuildProgram(mProgram, 0, nullptr, nullptr, nullptr, nullptr);
    checkClResult(err, "clBuildProgram");

    mKrnHitTriangle = clCreateKernel(mProgram, "triangleHit", &err);
    checkClResult(err, "clCreateKernel (triangleHit)");

    mKrnHitSphere = clCreateKernel(mProgram, "sphereHit", &err);
    checkClResult(err, "clCreateKernel (sphereHit)");

    /* process scene data */
    mTriangleCount = scene.triangles.size();
    if (mTriangleCount > 0) {
        mTriangles = new cl_float[mTriangleCount * TRIANGLE_SIZE];

        mMemTriangles = clCreateBuffer(
                mContext, CL_MEM_READ_ONLY, sizeof(cl_float) * TRIANGLE_SIZE * mTriangleCount, nullptr, &err
        );
        checkClResult(err, "clCreateBuffer (triangles)");

        for (size_t i = 0; i < mTriangleCount; i++) {
            putTriangle(*scene.triangles[i], mTriangles + (i * 12));
        }

        err = clEnqueueWriteBuffer(
                mCommandQueue, mMemTriangles, CL_TRUE, 0,
                sizeof(cl_float) * TRIANGLE_SIZE * mTriangleCount,
                mTriangles, 0, nullptr, nullptr
        );
        checkClResult(err, "clEnqueueWriteBuffer (triangles write)");
    }

    mSphereCount = scene.spheres.size();
    if (mSphereCount > 0) {
        mSpheres = new cl_float[mSphereCount * SPHERE_SIZE];

        mMemSpheres = clCreateBuffer(
                mContext, CL_MEM_READ_ONLY, sizeof(cl_float) * SPHERE_SIZE * mSphereCount, nullptr, &err
        );
        checkClResult(err, "clCreateBuffer (spheres)");

        for (size_t i = 0; i < scene.spheres.size(); i++) {
            putSphere(*scene.spheres[i], mSpheres + (i * 4));
        }

        err = clEnqueueWriteBuffer(
                mCommandQueue, mMemSpheres, CL_TRUE, 0,
                sizeof(cl_float) * SPHERE_SIZE * mSphereCount,
                mSpheres, 0, nullptr, nullptr
        );
        checkClResult(err, "clEnqueueWriteBuffer (spheres write)");
    }
}

OpenClExecutor::~OpenClExecutor() {
    if (mMemSpheres != 0) {
        clReleaseMemObject(mMemSpheres);
        mMemSpheres = 0;
    }
    if (mMemTriangles != 0) {
        clReleaseMemObject(mMemTriangles);
        mMemTriangles = 00;
    }
    if (mKrnHitSphere != 0) {
        clReleaseKernel(mKrnHitSphere);
        mKrnHitSphere = 0;
    }
    if (mKrnHitTriangle != 0) {
        clReleaseKernel(mKrnHitTriangle);
        mKrnHitTriangle = 0;
    }
    if (mProgram != 0) {
        clReleaseProgram(mProgram);
        mProgram = 0;
    }
    if (mCommandQueue != 0) {
        clReleaseCommandQueue(mCommandQueue);
        mCommandQueue = 0;
    }
    if (mContext != 0) {
        clReleaseContext(mContext);
        mContext = 0;
    }
    if (mTriangles != nullptr) {
        delete[] mTriangles;
        mTriangles = nullptr;
    }
    if (mSpheres != nullptr) {
        delete[] mSpheres;
        mSpheres = nullptr;
    }
}

void
OpenClExecutor::computeClosestHitTriangle(
        const cl_float *rays,
        cl_uint rayCount,
        std::vector<std::tuple<TriangleHit, size_t>> &resHits
) {
    cl_int err;

    resHits.clear();
    resHits.resize(rayCount, std::make_tuple(TriangleHit(false), 0));

    if (mTriangleCount == 0 || rayCount == 0) {
        return;
    }

    cl_mem memRays = clCreateBuffer(
            mContext, CL_MEM_READ_ONLY, sizeof(cl_float) * RAY_SIZE * rayCount, nullptr, &err
    );
    checkClResult(err, "closestHitTriangle clCreateBuffer (memRays)");

    cl_mem memTrianglesHits = clCreateBuffer(
            mContext, CL_MEM_WRITE_ONLY, sizeof(cl_char) * mTriangleCount * rayCount, nullptr, &err
    );
    checkClResult(err, "closestHitTriangle clCreateBuffer (hits)");

    cl_mem memTrianglesHitParams = clCreateBuffer(
            mContext, CL_MEM_WRITE_ONLY, sizeof(cl_float) * TRIANGLE_HIT_PARAM_SIZE * mTriangleCount * rayCount,
            nullptr,
            &err
    );
    checkClResult(err, "closestHitTriangle clCreateBuffer (hitParams)");

    err = clEnqueueWriteBuffer(
            mCommandQueue, memRays, CL_TRUE, 0, sizeof(cl_float) * RAY_SIZE * rayCount, rays, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (closestHit triangle rays)");

    cl_uint triangleCount = (cl_uint) mTriangleCount;
    clSetKernelArg(mKrnHitTriangle, 0, sizeof(cl_mem), &mMemTriangles);
    clSetKernelArg(mKrnHitTriangle, 1, sizeof(cl_uint), &triangleCount);
    clSetKernelArg(mKrnHitTriangle, 2, sizeof(cl_mem), &memRays);
    clSetKernelArg(mKrnHitTriangle, 3, sizeof(cl_uint), &rayCount);
    clSetKernelArg(mKrnHitTriangle, 4, sizeof(cl_mem), &memTrianglesHits);
    clSetKernelArg(mKrnHitTriangle, 5, sizeof(cl_mem), &memTrianglesHitParams);

    size_t dimensions[] = {mTriangleCount, rayCount};
    err = clEnqueueNDRangeKernel(
            mCommandQueue, mKrnHitTriangle, 2, nullptr, dimensions, nullptr, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueNDRangeKernel (closestHit triangles)");

    std::vector<cl_char> triangleHits(rayCount * mTriangleCount);
    err = clEnqueueReadBuffer(
            mCommandQueue, memTrianglesHits, CL_TRUE, 0,
            sizeof(cl_char) * mTriangleCount * rayCount, triangleHits.data(), 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (closestHit triangles hit)");

    std::vector<cl_float> triangleHitParams(TRIANGLE_HIT_PARAM_SIZE * rayCount * mTriangleCount);
    err = clEnqueueReadBuffer(
            mCommandQueue, memTrianglesHitParams, CL_TRUE, 0,
            sizeof(cl_float) * TRIANGLE_HIT_PARAM_SIZE * mTriangleCount * rayCount, triangleHitParams.data(), 0,
            nullptr,
            nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (closestHit triangles hit params)");


    for (size_t i = 0; i < rayCount; i++) {
        glm::vec3 rayFrom(
                rays[i * RAY_SIZE],
                rays[i * RAY_SIZE + 1],
                rays[i * RAY_SIZE + 2]
        );

        glm::vec3 rayDir(
                rays[i * RAY_SIZE + 3],
                rays[i * RAY_SIZE + 4],
                rays[i * RAY_SIZE + 5]
        );

        bool retIsHit = false;
        size_t retI = 0;
        cl_float retT = 0.0f;
        cl_float retU = 0.0f;
        cl_float retV = 0.0f;
        glm::vec3 retPt;
        glm::vec3 retNorm;

        cl_float *paramsForRay = triangleHitParams.data() + (i * mTriangleCount * TRIANGLE_HIT_PARAM_SIZE);
        for (size_t j = 0; j < mTriangleCount; j++) {
            cl_float *hitParams = paramsForRay + (j * TRIANGLE_HIT_PARAM_SIZE);
            cl_float t = hitParams[0];
            if (triangleHits[i * mTriangleCount + j] && (!retIsHit || t < retT)) {
                retIsHit = true;
                retT = t;
                retU = hitParams[1];
                retV = hitParams[2];
                retPt.x = hitParams[3];
                retPt.y = hitParams[4];
                retPt.z = hitParams[5];
                retNorm.x = hitParams[6];
                retNorm.y = hitParams[7];
                retNorm.z = hitParams[8];
                retI = j;
            }
        }
        if (retIsHit) {
            resHits[i] = std::make_tuple(TriangleHit(true, retT, retU, retV, retPt, retNorm), retI);
        }
    }

    clReleaseMemObject(memRays);
    clReleaseMemObject(memTrianglesHits);
    clReleaseMemObject(memTrianglesHitParams);
}

void OpenClExecutor::computeAnyHitTriangle(
        const cl_float *rays,
        cl_uint rayCount,
        cl_char *resHits
) {
    cl_int err;

    if (mTriangleCount == 0 || rayCount == 0) {
        return;
    }

    cl_mem memRays = clCreateBuffer(
            mContext, CL_MEM_READ_ONLY, sizeof(cl_float) * RAY_SIZE * rayCount, nullptr, &err
    );
    checkClResult(err, "anyHitTriangle clCreateBuffer (memRays)");

    cl_mem memTrianglesHits = clCreateBuffer(
            mContext, CL_MEM_WRITE_ONLY, sizeof(cl_char) * mTriangleCount * rayCount, nullptr, &err
    );
    checkClResult(err, "anyHitTriangle clCreateBuffer (hits)");

    cl_mem memTrianglesHitParams = clCreateBuffer(
            mContext, CL_MEM_WRITE_ONLY, sizeof(cl_float) * TRIANGLE_HIT_PARAM_SIZE * mTriangleCount * rayCount,
            nullptr,
            &err
    );
    checkClResult(err, "anyHitTriangle clCreateBuffer (hitParams)");

    err = clEnqueueWriteBuffer(
            mCommandQueue, memRays, CL_TRUE, 0, sizeof(cl_float) * RAY_SIZE * rayCount, rays, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (anyHit triangle rays)");

    cl_uint triangleCount = (cl_uint) mTriangleCount;
    clSetKernelArg(mKrnHitTriangle, 0, sizeof(cl_mem), &mMemTriangles);
    clSetKernelArg(mKrnHitTriangle, 1, sizeof(cl_uint), &triangleCount);
    clSetKernelArg(mKrnHitTriangle, 2, sizeof(cl_mem), &memRays);
    clSetKernelArg(mKrnHitTriangle, 3, sizeof(cl_uint), &rayCount);
    clSetKernelArg(mKrnHitTriangle, 4, sizeof(cl_mem), &memTrianglesHits);
    clSetKernelArg(mKrnHitTriangle, 5, sizeof(cl_mem), &memTrianglesHitParams);

    size_t dimensions[] = {mTriangleCount, rayCount};
    err = clEnqueueNDRangeKernel(
            mCommandQueue, mKrnHitTriangle, 2, nullptr, dimensions, nullptr, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueNDRangeKernel (anyHit triangles)");

    std::vector<cl_char> triangleHits(rayCount * mTriangleCount);
    err = clEnqueueReadBuffer(
            mCommandQueue, memTrianglesHits, CL_TRUE, 0,
            sizeof(cl_char) * mTriangleCount * rayCount, triangleHits.data(), 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (anyHit triangles hit)");

    for (size_t i = 0; i < rayCount; i++) {
        bool hits = false;

        for (size_t j = 0; j < mTriangleCount; j++) {
            if (hits) {
                break;
            }

            if (triangleHits[i * mTriangleCount + j]) {
                hits = true;
            }
        }

        resHits[i] = hits;
    }

    clReleaseMemObject(memRays);
    clReleaseMemObject(memTrianglesHits);
    clReleaseMemObject(memTrianglesHitParams);
}

void
OpenClExecutor::computeClosestHitSphere(
        const cl_float *rays,
        cl_uint rayCount,
        std::vector<std::tuple<SphereHit, size_t>> &resHits
) {
    cl_int err;

    resHits.clear();
    resHits.resize(rayCount, std::make_tuple(SphereHit(false), 0));

    if (mSphereCount == 0 || rayCount == 0) {
        return;
    }

    cl_mem memRays = clCreateBuffer(
            mContext, CL_MEM_READ_ONLY, sizeof(cl_float) * RAY_SIZE * rayCount, nullptr, &err
    );
    checkClResult(err, "closestHitSphere clCreateBuffer (memRays)");

    cl_mem memSpheresHits = clCreateBuffer(
            mContext, CL_MEM_WRITE_ONLY, sizeof(cl_char) * mSphereCount * rayCount, nullptr, &err
    );
    checkClResult(err, "closestHitSphere clCreateBuffer (hits)");

    cl_mem memSpheresHitParams = clCreateBuffer(
            mContext, CL_MEM_WRITE_ONLY, sizeof(cl_float) * SPHERE_HIT_PARAM_SIZE * mSphereCount * rayCount, nullptr,
            &err
    );
    checkClResult(err, "closestHitSphere clCreateBuffer (hitParams)");

    err = clEnqueueWriteBuffer(
            mCommandQueue, memRays, CL_TRUE, 0, sizeof(cl_float) * RAY_SIZE * rayCount, rays, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (closestHit spheres rays)");

    cl_uint sphereCount = (cl_uint) mSphereCount;
    clSetKernelArg(mKrnHitSphere, 0, sizeof(cl_mem), &mMemSpheres);
    clSetKernelArg(mKrnHitSphere, 1, sizeof(cl_uint), &sphereCount);
    clSetKernelArg(mKrnHitSphere, 2, sizeof(cl_mem), &memRays);
    clSetKernelArg(mKrnHitSphere, 3, sizeof(cl_uint), &rayCount);
    clSetKernelArg(mKrnHitSphere, 4, sizeof(cl_mem), &memSpheresHits);
    clSetKernelArg(mKrnHitSphere, 5, sizeof(cl_mem), &memSpheresHitParams);

    size_t dimensions[] = {mSphereCount, rayCount};
    err = clEnqueueNDRangeKernel(
            mCommandQueue, mKrnHitSphere, 2, nullptr, dimensions, nullptr, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueNDRangeKernel (closestHit spheres)");

    std::vector<cl_char> sphereHits(rayCount * mSphereCount);
    err = clEnqueueReadBuffer(
            mCommandQueue, memSpheresHits, CL_TRUE, 0,
            sizeof(cl_char) * mSphereCount * rayCount, sphereHits.data(), 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (closestHit spheres hit)");

    std::vector<cl_float> sphereHitParams(SPHERE_HIT_PARAM_SIZE * rayCount * mSphereCount);
    err = clEnqueueReadBuffer(
            mCommandQueue, memSpheresHitParams, CL_TRUE, 0,
            sizeof(cl_float) * SPHERE_HIT_PARAM_SIZE * mSphereCount * rayCount, sphereHitParams.data(), 0, nullptr,
            nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (closestHit spheres hit params)");

    for (size_t i = 0; i < rayCount; i++) {
        glm::vec3 rayFrom(
                rays[i * RAY_SIZE],
                rays[i * RAY_SIZE + 1],
                rays[i * RAY_SIZE + 2]
        );

        glm::vec3 rayDir(
                rays[i * RAY_SIZE + 3],
                rays[i * RAY_SIZE + 4],
                rays[i * RAY_SIZE + 5]
        );

        bool retIsHit = false;
        size_t retI = 0;
        cl_float retT = 0.0f;
        glm::vec3 retPt;
        glm::vec3 retNorm;

        cl_float *paramsForRay = sphereHitParams.data() + (i * mSphereCount * SPHERE_HIT_PARAM_SIZE);
        for (size_t j = 0; j < mSphereCount; j++) {
            cl_float *hitParams = paramsForRay + (j * SPHERE_HIT_PARAM_SIZE);
            cl_float t = hitParams[0];
            if (sphereHits[i * mSphereCount + j] && (!retIsHit || t < retT)) {
                retIsHit = true;
                retT = t;
                retI = j;
                retPt.x = hitParams[1];
                retPt.y = hitParams[2];
                retPt.z = hitParams[3];
                retNorm.x = hitParams[4];
                retNorm.y = hitParams[5];
                retNorm.z = hitParams[6];
            }
        }
        if (retIsHit) {
            resHits[i] = std::make_tuple(SphereHit(true, retT, retPt, retNorm), retI);
        }
    }

    clReleaseMemObject(memRays);
    clReleaseMemObject(memSpheresHits);
    clReleaseMemObject(memSpheresHitParams);
}

void
OpenClExecutor::computeAnyHitSphere(
        const cl_float *rays,
        cl_uint rayCount,
        cl_char *resHits
) {
    cl_int err;

    if (mSphereCount == 0 || rayCount == 0) {
        return;
    }

    cl_mem memRays = clCreateBuffer(
            mContext, CL_MEM_READ_ONLY, sizeof(cl_float) * RAY_SIZE * rayCount, nullptr, &err
    );
    checkClResult(err, "closestHitSphere clCreateBuffer (memRays)");

    cl_mem memSpheresHits = clCreateBuffer(
            mContext, CL_MEM_WRITE_ONLY, sizeof(cl_char) * mSphereCount * rayCount, nullptr, &err
    );
    checkClResult(err, "closestHitSphere clCreateBuffer (hits)");

    cl_mem memSpheresHitParams = clCreateBuffer(
            mContext, CL_MEM_WRITE_ONLY, sizeof(cl_float) * SPHERE_HIT_PARAM_SIZE * mSphereCount * rayCount, nullptr,
            &err
    );
    checkClResult(err, "closestHitSphere clCreateBuffer (hitParams)");

    err = clEnqueueWriteBuffer(
            mCommandQueue, memRays, CL_TRUE, 0, sizeof(cl_float) * RAY_SIZE * rayCount, rays, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (closestHit spheres rays)");

    cl_uint sphereCount = (cl_uint) mSphereCount;
    clSetKernelArg(mKrnHitSphere, 0, sizeof(cl_mem), &mMemSpheres);
    clSetKernelArg(mKrnHitSphere, 1, sizeof(cl_uint), &sphereCount);
    clSetKernelArg(mKrnHitSphere, 2, sizeof(cl_mem), &memRays);
    clSetKernelArg(mKrnHitSphere, 3, sizeof(cl_uint), &rayCount);
    clSetKernelArg(mKrnHitSphere, 4, sizeof(cl_mem), &memSpheresHits);
    clSetKernelArg(mKrnHitSphere, 5, sizeof(cl_mem), &memSpheresHitParams);

    size_t dimensions[] = {mSphereCount, rayCount};
    err = clEnqueueNDRangeKernel(
            mCommandQueue, mKrnHitSphere, 2, nullptr, dimensions, nullptr, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueNDRangeKernel (closestHit spheres)");

    std::vector<cl_char> sphereHits(rayCount * mSphereCount);
    err = clEnqueueReadBuffer(
            mCommandQueue, memSpheresHits, CL_TRUE, 0,
            sizeof(cl_char) * mSphereCount * rayCount, sphereHits.data(), 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (closestHit spheres hit)");

    for (size_t i = 0; i < rayCount; i++) {
        bool hits = false;

        for (size_t j = 0; j < mSphereCount; j++) {
            if (hits) {
                break;
            }

            if (sphereHits[i * mSphereCount + j]) {
                hits = true;
            }
        }

        resHits[i] = hits;
    }

    clReleaseMemObject(memRays);
    clReleaseMemObject(memSpheresHits);
    clReleaseMemObject(memSpheresHitParams);
}
