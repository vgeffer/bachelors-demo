#pragma once

#include <glm/ext/quaternion_exponential.hpp>
#include <glm/geometric.hpp>
#if defined (__cplusplus) 

    #include "../../lib/glpix.hpp"
    #include <glm/ext/vector_float4.hpp>
    #include <cmath>


    using float2 = cl_float2; using float3 = cl_float3; using float4 = cl_float4;
    using int2 = cl_int2;     using int3 = cl_int3;     using int4 = cl_int4;

#endif

#define RT_EPSILON 0.001f
#define BVH_CHILD_STOP_COUNT 4
#define BVH_STACK_SIZE 128
#define MIN_CHILDREN_ALPHA_SUM 0.01

typedef struct __attribute__((packed)) _ray {
	float4 origin, dir; 
} ray;
 

typedef struct __attribute__((packed)) _ray_hit {
    int primitive;
    float distance;
    float4 point;
} hit;

/* Sphere BVH node */
typedef struct __attribute__((packed)) _bounding_sphere {

    float4 center;
    float rsquared;
    int cbegin, cend;

    #if defined (__cplusplus) 

        float count = 1; 
        float2 pad;
        float alpha;

        void grow(const _bounding_sphere& sp) {
            glm::vec4 new_pos = (glm::vec4(center.x, center.y, center.z, 1) * count + 
                                 glm::vec4(sp.center.x, sp.center.y, sp.center.z, 1)) / (count + 1);
            center = (float4) { .x = new_pos.x, .y = new_pos.y, .z = new_pos.z, .w = 1 };
            count += 1;

            float dist = glm::distance(new_pos, glm::vec4(sp.center.x, sp.center.y, sp.center.z, sp.center.w));
            float r = glm::sqrt(sp.rsquared);

            if ((dist + r) * (dist + r) > rsquared)
                rsquared = (dist + r) * (dist + r);
        }
    
    #else
        float4 color;
    #endif

} sphere;

