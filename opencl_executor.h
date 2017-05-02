//
// Created by vlad on 5/1/17.
//

#ifndef RAY_TRACING_OPENCL_EXECUTOR_H
#define RAY_TRACING_OPENCL_EXECUTOR_H

#include "CL/cl.h"
#include "scene.h"


class OpenClExecutor {
    const size_t TRIANGLE_SIZE = 12;
    const size_t TRIANGLE_HIT_PARAM_SIZE = 9;
    const size_t SPHERE_SIZE = 4;
    const size_t SPHERE_HIT_PARAM_SIZE = 7;

    cl_context mContext;
    cl_command_queue mCommandQueue;
    cl_program mProgram;

    cl_kernel mKrnHitTriangle;
    size_t mTriangleCount;
    cl_mem mMemTriangles;
    cl_mem mMemOutTrianglesHit;
    cl_mem mMemOutTrianglesHitParams;
    cl_float *mTriangles;
    cl_char *mOutTrianglesHit;
    cl_float *mOutTrianglesHitParams;

    cl_kernel mKrnHitSphere;
    size_t mSphereCount;
    cl_mem mMemSpheres;
    cl_mem mMemOutSpheresHit;
    cl_mem mMemOutSpheresHitParams;
    cl_float *mSpheres;
    cl_char *mOutSpheresHit;
    cl_float *mOutSpheresHitParams;

    cl_mem mMemRayFrom;
    cl_mem mMemRayDir;

public:
    OpenClExecutor(const Scene &scene);

    ~OpenClExecutor();

    TriangleHit computeClosestHitTriangle(const glm::vec3 &rayFrom, const glm::vec3 &rayDir);

    SphereHit computeClosestHitSphere(const glm::vec3 &rayFrom, const glm::vec3 &rayDir);

    bool computeAnyHitTriangle(const glm::vec3 &rayFrom, const glm::vec3 &rayDir);

    bool computeAnyHitSphere(const glm::vec3 &rayFrom, const glm::vec3 &rayDir);

private:
    void checkClResult(cl_int err, const char *msg) {
        if (err != CL_SUCCESS) {
            std::cerr << msg << ": " << std::endl;
            throw std::runtime_error("OpenCl function failed");
        }
    }

    void putTriangle(const Triangle &triangle, float *dst) {
        dst[0] = triangle.p.x;
        dst[1] = triangle.p.y;
        dst[2] = triangle.p.z;
        dst[3] = triangle.e1.x;
        dst[4] = triangle.e1.y;
        dst[5] = triangle.e1.z;
        dst[6] = triangle.e2.x;
        dst[7] = triangle.e2.y;
        dst[8] = triangle.e2.z;
        dst[9] = triangle.norm.x;
        dst[10] = triangle.norm.y;
        dst[11] = triangle.norm.z;
    }

    void putSphere(const Sphere &sphere, float *dst) {
        dst[0] = sphere.center.x;
        dst[1] = sphere.center.y;
        dst[2] = sphere.center.z;
        dst[4] = sphere.radius;
    }
};


#endif //RAY_TRACING_OPENCL_EXECUTOR_H
