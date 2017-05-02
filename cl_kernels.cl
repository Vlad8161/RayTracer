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
*/
__kernel void
triangleHit(
    const __constant float* triangles,
    const __global unsigned int triangleCount,
    const __global float3 rayFrom,
    const __global float3 rayDir,
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
*/
__kernel void
sphereHit(
    const __constant float* mSpheres,
    const __global unsigned int sphereCount,
    const __global float3 rayFrom,
    const __global float3 rayDir,
    __global char* retHit,
    __global float* retHitParams
);


float
dot(
    float3 v1,
    float3 v2,
);


float3
cross(
    float3 v1,
    float3 v2,
);


__kernel void
triangleHit(
    const __constant float* triangles,
    const __global unsigned int triangleCount,
    const __global float3 rayFrom,
    const __global float3 rayDir,
    __global char* retHits,
    __global float* retHitParams
) {
/*    unsigned int id = get_global_id(0);

    if (id >= triangleCount) {
        return;
    }

    float *triangle = triangles + (id * 12);
    float3 p = vload3(0, triangle);
    float3 e1 = vload3(1, triangle);
    float3 e2 = vload3(2, triangle);
    float3 norm = vload3(3, triangle);

    float3 q = rayFrom - p;
    float det = dot(rayDir, cross(e2, e1));
    float dett = dot(q, cross(e1, e2));
    float detu = dot(rayDir, cross(e2, q));
    float detv = dot(rayDir, cross(q, e1));

    float t = dett / det;
    float u = detu / det;
    float v = detv / det;
    float3 hitPt = rayFrom + t * rayDir;

    char retHit = (det > 0.001f && det < -0.001f)
    retHit = retHit && (t > 0.001f);
    retHit = retHit && (u > 0.0f && v > 0.0f && u + v < 1.0f);

    retHits[id] = retHit;
    float* retHitParam = retHitParams + (id * 9);
    retHitParam[0] = t;
    retHitParam[1] = u;
    retHitParam[2] = v;
    retHitParam[3] = hitPt.x;
    retHitParam[4] = hitPt.y;
    retHitParam[5] = hitPt.z;
    retHitParam[6] = norm.x;
    retHitParam[7] = norm.y;
    retHitParam[8] = norm.z;*/
}


/**
  struct Sphere
    - vec3 center
    - vec3 radius

  struct HitParam
    - float t
    - vec3 hitPt
    - vec3 norm
*/
__kernel void
sphereHit(
    const __constant float* mSpheres,
    const __global unsigned int sphereCount,
    const __global float3 rayFrom,
    const __global float3 rayDir,
    __global char* retHit,
    __global float* retHitParams
) {

}


float
dot(
    float3 v1,
    float3 v2,
) {
    return 0.0f;
}


float3
cross(
    float3 v1,
    float3 v2,
) {
    float3 ret;
    ret.x = 0.0f;
    ret.y = 0.0f;
    ret.z = 0.0f;
    return ret;
}
