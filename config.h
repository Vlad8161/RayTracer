//
// Created by vlad on 4/29/17.
//

#ifndef RAY_TRACING_CONFIG_H
#define RAY_TRACING_CONFIG_H

#define WIDTH (800)
#define HEIGHT (600)
//#define RENDER_PARALLEL
#define GPU_ACCELERATION
//#define ENABLE_AO
//#define ENABLE_REFLECTION
#define RENDER_COUNT (1)
#define AO_RAYS_COUNT (30)
#define MAX_REFLECTION_DEPTH (2)
#define THREAD_POOL_SIZE (1)
#define SUB_BLOCK_WIDTH (48)
#define SUB_BLOCK_HEIGHT (48)
#define EPS (0.0001)

#endif //RAY_TRACING_CONFIG_H
