//
// Created by vlad on 5/1/17.
//

#include <fstream>
#include "opencl_executor.h"

OpenClExecutor::OpenClExecutor(const Scene &scene) {
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
        mOutTrianglesHit = new cl_char[mTriangleCount];
        mOutTrianglesHitParams = new cl_float[mTriangleCount * TRIANGLE_HIT_PARAM_SIZE];

        mMemTriangles = clCreateBuffer(
                mContext, CL_MEM_READ_ONLY, sizeof(cl_float) * TRIANGLE_SIZE * mTriangleCount, nullptr, &err
        );
        checkClResult(err, "clCreateBuffer (triangles)");

        mMemOutTrianglesHit = clCreateBuffer(
                mContext, CL_MEM_WRITE_ONLY, sizeof(cl_char) * mTriangleCount, nullptr, &err
        );
        checkClResult(err, "clCreateBuffer (triangles hit)");

        mMemOutTrianglesHitParams = clCreateBuffer(
                mContext, CL_MEM_WRITE_ONLY, sizeof(cl_float) * TRIANGLE_HIT_PARAM_SIZE * mTriangleCount, nullptr, &err
        );
        checkClResult(err, "clCreateBuffer (triangles hit params)");

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
        mOutSpheresHit = new cl_char[mSphereCount];
        mOutSpheresHitParams = new cl_float[mSphereCount * SPHERE_HIT_PARAM_SIZE];

        mMemSpheres = clCreateBuffer(
                mContext, CL_MEM_READ_ONLY, sizeof(cl_float) * SPHERE_SIZE * mSphereCount, nullptr, &err
        );
        checkClResult(err, "clCreateBuffer (spheres)");

        mMemOutSpheresHit = clCreateBuffer(
                mContext, CL_MEM_WRITE_ONLY, sizeof(cl_char) * mSphereCount, nullptr, &err
        );
        checkClResult(err, "clCreateBuffer (spheres hit)");

        mMemOutSpheresHitParams = clCreateBuffer(
                mContext, CL_MEM_WRITE_ONLY, sizeof(cl_float) * SPHERE_HIT_PARAM_SIZE * mSphereCount, nullptr, &err
        );
        checkClResult(err, "clCreateBuffer (spheres hit params)");

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

    mMemRayFrom = clCreateBuffer(
            mContext, CL_MEM_READ_ONLY, sizeof(cl_float) * 3, nullptr, &err
    );
    checkClResult(err, "clCreateBuffer (rayFrom)");

    mMemRayDir = clCreateBuffer(
            mContext, CL_MEM_READ_ONLY, sizeof(cl_float) * 3, nullptr, &err
    );
    checkClResult(err, "clCreateBuffer (rayDir)");
}

OpenClExecutor::~OpenClExecutor() {
    if (mMemRayFrom != 0) {
        clReleaseMemObject(mMemRayFrom);
        mMemRayFrom = 0;
    }
    if (mMemRayDir != 0) {
        clReleaseMemObject(mMemRayDir);
        mMemRayDir = 0;
    }
    if (mMemSpheres != 0) {
        clReleaseMemObject(mMemSpheres);
        mMemSpheres = 0;
    }
    if (mMemOutSpheresHit != 0) {
        clReleaseMemObject(mMemOutSpheresHit);
        mMemOutSpheresHit = 0;
    }
    if (mMemOutSpheresHitParams != 0) {
        clReleaseMemObject(mMemOutSpheresHitParams);
        mMemOutSpheresHitParams = 0;
    }
    if (mMemTriangles != 0) {
        clReleaseMemObject(mMemTriangles);
        mMemTriangles = 00;
    }
    if (mMemOutTrianglesHit != 0) {
        clReleaseMemObject(mMemOutTrianglesHit);
        mMemOutTrianglesHit = 0;
    }
    if (mMemOutTrianglesHitParams) {
        clReleaseMemObject(mMemOutTrianglesHitParams);
        mMemOutTrianglesHitParams = 0;
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
    if (mOutTrianglesHit != nullptr) {
        delete[] mOutTrianglesHit;
        mOutTrianglesHit = nullptr;
    }
    if (mOutTrianglesHitParams != nullptr) {
        delete[] mOutTrianglesHitParams;
        mOutTrianglesHitParams = nullptr;
    }
    if (mSpheres != nullptr) {
        delete[] mSpheres;
        mSpheres = nullptr;
    }
    if (mOutSpheresHit != nullptr) {
        delete[] mOutSpheresHit;
        mOutSpheresHit = nullptr;
    }
    if (mOutSpheresHitParams != nullptr) {
        delete[] mOutSpheresHitParams;
        mOutSpheresHitParams = nullptr;
    }
}

std::tuple<TriangleHit, int> OpenClExecutor::computeClosestHitTriangle(const glm::vec3 &rayFrom, const glm::vec3 &rayDir) {
    cl_int err;

    if (mTriangleCount == 0) {
        return std::make_tuple(TriangleHit(false), 0);
    }

    cl_float rayFromCl[3] = {rayFrom.x, rayFrom.y, rayFrom.z};
    err = clEnqueueWriteBuffer(
            mCommandQueue, mMemRayFrom, CL_TRUE, 0, sizeof(cl_float) * 3, rayFromCl, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (closestHit triangle rayFrom)");

    cl_float rayDirCl[3] = {rayDir.x, rayDir.y, rayDir.z};
    err = clEnqueueWriteBuffer(
            mCommandQueue, mMemRayDir, CL_TRUE, 0, sizeof(cl_float) * 3, rayDirCl, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (closestHit triangle rayDir)");

    cl_uint triangleCount = (cl_uint) mTriangleCount;
    std::cout << triangleCount << std::endl;
    err = clSetKernelArg(mKrnHitTriangle, 0, sizeof(cl_mem), &mMemTriangles);
    std::cout << err << std::endl;
    err = clSetKernelArg(mKrnHitTriangle, 1, sizeof(cl_uint), &triangleCount);
    std::cout << err << std::endl;
    err = clSetKernelArg(mKrnHitTriangle, 2, sizeof(cl_mem), &mMemRayFrom);
    std::cout << err << std::endl;
    err = clSetKernelArg(mKrnHitTriangle, 3, sizeof(cl_mem), &mMemRayDir);
    std::cout << err << std::endl;
    err = clSetKernelArg(mKrnHitTriangle, 4, sizeof(cl_mem), &mMemOutTrianglesHit);
    std::cout << err << std::endl;
    err = clSetKernelArg(mKrnHitTriangle, 5, sizeof(cl_mem), &mMemOutTrianglesHitParams);
    std::cout << err << std::endl;

    err = clEnqueueNDRangeKernel(
            mCommandQueue, mKrnHitTriangle, 1, nullptr, &mTriangleCount, nullptr, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueNDRangeKernel (closestHit triangles)");

    err = clEnqueueReadBuffer(
            mCommandQueue, mMemOutTrianglesHit, CL_TRUE, 0,
            sizeof(cl_char) * mTriangleCount, mOutTrianglesHit, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (closestHit triangles hit)");

    err = clEnqueueReadBuffer(
            mCommandQueue, mMemOutTrianglesHitParams, CL_TRUE, 0,
            sizeof(cl_float) * TRIANGLE_HIT_PARAM_SIZE * mTriangleCount, mOutTrianglesHitParams, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (closestHit triangles hit params)");

    bool retIsHit = false;
    int retI = 0;
    float retT = 0.0f;
    float retU = 0.0f;
    float retV = 0.0f;
    glm::vec3 retPt;
    glm::vec3 retNorm;
    for (size_t i = 0; i < mTriangleCount; i++) {
        float* hitParams = mOutTrianglesHitParams + (i * TRIANGLE_HIT_PARAM_SIZE);
        float t = hitParams[0];
        if (mOutTrianglesHit[i] && (!retIsHit || t < retT)) {
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
            retI = i;
        }
    }

    if (!retIsHit) {
        return std::make_tuple(TriangleHit(false), 0);
    } else {
        return std::make_tuple(TriangleHit(true, retT, retU, retV, retPt, retNorm), retI);
    }
}

bool OpenClExecutor::computeAnyHitTriangle(const glm::vec3 &rayFrom, const glm::vec3 &rayDir) {
    /* triangles */
    cl_int err;

    if (mTriangleCount == 0) {
        return false;
    }

    cl_float rayFromCl[3] = {rayFrom.x, rayFrom.y, rayFrom.z};
    err = clEnqueueWriteBuffer(
            mCommandQueue, mMemRayFrom, CL_TRUE, 0, sizeof(cl_float) * 3, rayFromCl, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (anyHit triangle rayFrom)");

    cl_float rayDirCl[3] = {rayDir.x, rayDir.y, rayDir.z};
    err = clEnqueueWriteBuffer(
            mCommandQueue, mMemRayDir, CL_TRUE, 0, sizeof(cl_float) * 3, rayDirCl, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (anyHit triangle rayDir)");

    cl_uint triangleCount = (cl_uint) mTriangleCount;
    clSetKernelArg(mKrnHitTriangle, 0, sizeof(cl_mem), &mMemTriangles);
    clSetKernelArg(mKrnHitTriangle, 1, sizeof(cl_uint), &triangleCount);
    clSetKernelArg(mKrnHitTriangle, 2, sizeof(cl_mem), &mMemRayFrom);
    clSetKernelArg(mKrnHitTriangle, 3, sizeof(cl_mem), &mMemRayDir);
    clSetKernelArg(mKrnHitTriangle, 4, sizeof(cl_mem), &mMemOutTrianglesHit);
    clSetKernelArg(mKrnHitTriangle, 5, sizeof(cl_mem), &mMemOutTrianglesHitParams);

    err = clEnqueueNDRangeKernel(
            mCommandQueue, mKrnHitTriangle, 1, nullptr, &mTriangleCount, nullptr, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueNDRangeKernel (anyHit triangles)");

    err = clEnqueueReadBuffer(
            mCommandQueue, mMemOutTrianglesHit, CL_TRUE, 0,
            sizeof(cl_char) * mTriangleCount, mOutTrianglesHit, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (anyHit triangles hit)");

    for (size_t i = 0; i < mTriangleCount; i++) {
        if (mOutTrianglesHit[i]) {
            return true;
        }
    }

    return false;
}

std::tuple<SphereHit, int> OpenClExecutor::computeClosestHitSphere(const glm::vec3 &rayFrom, const glm::vec3 &rayDir) {
    cl_int err;

    if (mSphereCount == 0) {
        return std::make_tuple(SphereHit(false), 0);
    }

    cl_float rayFromCl[3] = {rayFrom.x, rayFrom.y, rayFrom.z};
    err = clEnqueueWriteBuffer(
            mCommandQueue, mMemRayFrom, CL_TRUE, 0, sizeof(cl_float) * 3, rayFromCl, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (closestHitSphere rayFrom)");

    cl_float rayDirCl[3] = {rayDir.x, rayDir.y, rayDir.z};
    err = clEnqueueWriteBuffer(
            mCommandQueue, mMemRayDir, CL_TRUE, 0, sizeof(cl_float) * 3, rayDirCl, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (closestHitSphere rayDir)");

    cl_uint sphereCount = (cl_uint) mSphereCount;
    clSetKernelArg(mKrnHitSphere, 0, sizeof(cl_mem), &mMemSpheres);
    clSetKernelArg(mKrnHitSphere, 1, sizeof(cl_uint), &sphereCount);
    clSetKernelArg(mKrnHitSphere, 2, sizeof(cl_mem), &mMemRayFrom);
    clSetKernelArg(mKrnHitSphere, 3, sizeof(cl_mem), &mMemRayDir);
    clSetKernelArg(mKrnHitSphere, 4, sizeof(cl_mem), &mMemOutSpheresHit);
    clSetKernelArg(mKrnHitSphere, 5, sizeof(cl_mem), &mMemOutSpheresHitParams);

    err = clEnqueueNDRangeKernel(
            mCommandQueue, mKrnHitTriangle, 1, nullptr, &mSphereCount, nullptr, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueNDRangeKernel (closestHit spheres)");

    err = clEnqueueReadBuffer(
            mCommandQueue, mMemOutSpheresHit, CL_TRUE, 0,
            sizeof(cl_char) * mSphereCount, mOutSpheresHit, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (closestHit spheres hit)");

    err = clEnqueueReadBuffer(
            mCommandQueue, mMemOutSpheresHitParams, CL_TRUE, 0,
            sizeof(cl_float) * SPHERE_HIT_PARAM_SIZE * mSphereCount, mOutSpheresHitParams, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (closestHit spheres hit params)");

    bool retIsHit = false;
    int retI = 0;
    float retT = 0.0f;
    glm::vec3 retPt;
    glm::vec3 retNorm;
    for (size_t i = 0; i < mSphereCount; i++) {
        float* hitParams = mOutSpheresHitParams + (i * SPHERE_HIT_PARAM_SIZE);
        float t = hitParams[0];
        if (mOutSpheresHit[i] && (!retIsHit || t < retT)) {
            retIsHit = true;
            retI = i;
            retT = t;
            retPt.x = hitParams[1];
            retPt.y = hitParams[2];
            retPt.z = hitParams[3];
            retNorm.x = hitParams[4];
            retNorm.y = hitParams[5];
            retNorm.z = hitParams[6];
        }
    }

    if (!retIsHit) {
        return std::tuple<SphereHit, int>(SphereHit(false), 0);
    } else {
        return std::tuple<SphereHit, int>(SphereHit(true, retT, retPt, retNorm), retI);
    }
}

bool OpenClExecutor::computeAnyHitSphere(const glm::vec3 &rayFrom, const glm::vec3 &rayDir) {
    cl_int err;

    if (mSphereCount == 0) {
        return false;
    }

    cl_float rayFromCl[3] = {rayFrom.x, rayFrom.y, rayFrom.z};
    err = clEnqueueWriteBuffer(
            mCommandQueue, mMemRayFrom, CL_TRUE, 0, sizeof(cl_float) * 3, rayFromCl, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (anyHitSphere rayFrom)");

    cl_float rayDirCl[3] = {rayDir.x, rayDir.y, rayDir.z};
    err = clEnqueueWriteBuffer(
            mCommandQueue, mMemRayDir, CL_TRUE, 0, sizeof(cl_float) * 3, rayDirCl, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueWriteBuffer (anyHitSphere rayDir)");

    cl_uint sphereCount = (cl_uint) mSphereCount;
    clSetKernelArg(mKrnHitSphere, 0, sizeof(cl_mem), &mMemSpheres);
    clSetKernelArg(mKrnHitSphere, 1, sizeof(cl_uint), &sphereCount);
    clSetKernelArg(mKrnHitSphere, 2, sizeof(cl_mem), &mMemRayFrom);
    clSetKernelArg(mKrnHitSphere, 3, sizeof(cl_mem), &mMemRayDir);
    clSetKernelArg(mKrnHitSphere, 4, sizeof(cl_mem), &mMemOutSpheresHit);
    clSetKernelArg(mKrnHitSphere, 5, sizeof(cl_mem), &mMemOutSpheresHitParams);

    err = clEnqueueNDRangeKernel(
            mCommandQueue, mKrnHitTriangle, 1, nullptr, &mSphereCount, nullptr, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueNDRangeKernel (anyHit spheres)");

    err = clEnqueueReadBuffer(
            mCommandQueue, mMemOutSpheresHit, CL_TRUE, 0,
            sizeof(cl_char) * mSphereCount, mOutSpheresHit, 0, nullptr, nullptr
    );
    checkClResult(err, "clEnqueueReadBuffer (anyHit spheres hit)");

    for (size_t i = 0; i < mSphereCount; i++) {
        if (mOutSpheresHit[i]) {
            return true;
        }
    }

    return false;
}
