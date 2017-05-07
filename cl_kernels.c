//
// Created by vlad on 5/1/17.
//


/**
  struct Triangle
    - vec3 p,
    - vec3 e1,
    - vec3 e2,
    - vec3 norm

  struct HitParam
    - float t
    - float u
    - float v
    - vec3 hitPt
    - vec3 norm

  struct Ray
    - p_x
    - p_y
    - p_z
    - d_x
    - d_y
    - d_z
*/
__kernel void
triangleHit(
    const __global float* triangles,
    const unsigned int triangleCount,
    const __global float* raysVec,
    const unsigned int raysCount,
    __global char* retHits,
    __global float* retHitParams
);


/**
  struct Sphere
    - vec3 center
    - float radius

  struct HitParam
    - float t
    - vec3 hitPt
    - vec3 norm

  struct Ray
    - p_x
    - p_y
    - p_z
    - d_x
    - d_y
    - d_z
*/
__kernel void
sphereHit(
    const __global float* mSpheres,
    const unsigned int sphereCount,
    const __global float* raysVec,
    const unsigned int raysCount,
    __global char* retHit,
    __global float* retHitParams
);


/**
  struct Triangle
    - vec3 p,
    - vec3 e1,
    - vec3 e2,
    - vec3 norm

  struct HitParam
    - float t
    - float u
    - float v
    - vec3 hitPt
    - vec3 norm

  struct Ray
    - p_x
    - p_y
    - p_z
    - d_x
    - d_y
    - d_z
*/
__kernel void
triangleHit(
    const __global float* triangles,
    const unsigned int triangleCount,
    const __global float* raysVec,
    const unsigned int raysCount,
    __global char* retHits,
    __global float* retHitParams
) {
    unsigned int id = get_global_id(0);
    unsigned int iRay = get_global_id(1);

    if (id >= triangleCount || iRay >= raysCount) {
        return;
    }

    float3 rayFrom = vload3(iRay * 2, raysVec);
    float3 rayDir = vload3(iRay * 2 + 1, raysVec);
    __global float *triangle = triangles + (id * 12);
    float3 p = vload3(0, triangle);
    float3 e1 = vload3(1, triangle);
    float3 e2 = vload3(2, triangle);
    float3 norm = vload3(3, triangle);

    float3 q = rayFrom - p;
    float3 cross_e2_e1;
    float3 cross_e1_e2;
    float3 cross_e2_q;
    float3 cross_q_e1;
    float4 buf_e1 = (float4)(e1, 0.0f);
    float4 buf_e2 = (float4)(e2, 0.0f);
    float4 buf_q = (float4)(q, 0.0f);
    float4 buf_res;

    buf_res = cross(buf_e2, buf_e1);
    cross_e2_e1 = (float3)(buf_res.x, buf_res.y, buf_res.z);

    cross_e1_e2 = -cross_e2_e1;

    buf_res = cross(buf_e2, buf_q);
    cross_e2_q = (float3)(buf_res.x, buf_res.y, buf_res.z);

    buf_res = cross(buf_q, buf_e1);
    cross_q_e1 = (float3)(buf_res.x, buf_res.y, buf_res.z);

    float det = dot(rayDir, cross_e2_e1);
    float dett = dot(q, cross_e1_e2);
    float detu = dot(rayDir, cross_e2_q);
    float detv = dot(rayDir, cross_q_e1);

    float t = dett / det;
    float u = detu / det;
    float v = detv / det;
    float3 hitPt = rayFrom + t * rayDir;

    char retHit = (det > 0.001f || det < -0.001f);
    retHit = retHit && (t > 0.001f);
    retHit = retHit && (u > 0.0f && v > 0.0f && u + v < 1.0f);

    retHits[iRay * triangleCount + id] = retHit;
    __global float* retHitParam = retHitParams + (iRay * 9 * triangleCount) + (id * 9);
    retHitParam[0] = t;
    retHitParam[1] = u;
    retHitParam[2] = v;
    retHitParam[3] = hitPt.x;
    retHitParam[4] = hitPt.y;
    retHitParam[5] = hitPt.z;
    retHitParam[6] = norm.x;
    retHitParam[7] = norm.y;
    retHitParam[8] = norm.z;
}


/**
  struct Sphere
    - vec3 center
    - float radius

  struct HitParam
    - float t
    - vec3 hitPt
    - vec3 norm

  struct Ray
    - p_x
    - p_y
    - p_z
    - d_x
    - d_y
    - d_z
*/
__kernel void
sphereHit(
    const __global float* mSpheres,
    const unsigned int sphereCount,
    const __global float* raysVec,
    const unsigned int raysCount,
    __global char* retHit,
    __global float* retHitParams
) {
    unsigned int id = get_global_id(0);
    unsigned int iRay = get_global_id(1);

    if (id >= sphereCount || iRay >= raysCount) {
        return;
    }

    float3 rayFrom = vload3(iRay * 2, raysVec);
    float3 rayDir = vload3(iRay * 2 + 1, raysVec);

    __global float* sphere = mSpheres + (id * 4);
    float3 center = vload3(0, sphere);
    float radius = sphere[3];

    float3 v = rayFrom - center;
    float a = dot(rayDir, rayDir);
    float b = 2 * dot(v, rayDir);
    float c = dot(v, v) - (radius * radius);
    float d = (b * b) - (4 * a * c);

    float t = 0.0f;
    retHit[iRay * sphereCount + id] = 0;
    if (d < 0.0f) {
        return;
    } else if (d < 0.0001f && d > -0.000f) {
        t = -b / 2.0f * a;
        if (t < 0.0f) {
            return;
        }
    } else {
        float sqrtD = sqrt(d);
        float t1 = (-b + sqrtD) / (2.0f * a);
        float t2 = (-b - sqrtD) / (2.0f * a);
        if (t1 > 0.0001f && t2 > 0.0001f) {
            t = t1 < t2 ? t1 : t2;
        } else if (t1 > 0.0001f) {
            t = t1;
        } else if (t2 > 0.0001f) {
            t = t2;
        } else {
            return;
        }
    }

    retHit[iRay * sphereCount + id] = 1;
    float3 hitPt = rayFrom + t * rayDir;
    float3 norm = (hitPt - center) / radius;

    __global float* retHitParam = retHitParams + (iRay * 7 * sphereCount) + (id * 7);
    retHitParam[0] = t;
    retHitParam[1] = hitPt.x;
    retHitParam[2] = hitPt.y;
    retHitParam[3] = hitPt.z;
    retHitParam[4] = norm.x;
    retHitParam[5] = norm.y;
    retHitParam[6] = norm.z;
}

