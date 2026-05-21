#pragma once

#if defined (__cplusplus) 

    #include "../../lib/glpix.hpp"

    using float2 = cl_float2; using float3 = cl_float3; using float4 = cl_float4;
    using int2 = cl_int2;     using int3 = cl_int3;     using int4 = cl_int4;

#endif

#define RT_EPSILON 0.00001f
#define BVH_CHILD_STOP_COUNT 16
#define BVH_STACK_SIZE 32
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
    float4 color;

} sphere;

