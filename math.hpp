#pragma once
#include <glm/glm.hpp>
#include "lib/glpix.hpp"

typedef struct __attribute__((packed)) _mat4f {
    union {
        float     items[4][4]; /* column, row */
        cl_float4 columns[4]; 
    };
} mat4f;

/* Vector types */
template <>
struct glpix::type_helper<glm::vec2> { 
    using type = cl_float2; 
    inline static const type cast(const glm::vec2& val) { return cl_float2 { .x = val.x, .y = val.y }; } 
};

template<> 
struct glpix::type_helper<glm::vec3> {
    using type = cl_float3; 
    inline static const type cast(const glm::vec3& val) { return cl_float3 { .x = val.x, .y = val.y, .z = val.z }; }
};

template <>
struct glpix::type_helper<glm::vec4> { 
    using type = cl_float4; 
    inline static const type cast(const glm::vec4& val) { return cl_float4 { .x = val.x, .y = val.y, .z = val.z, .w = val.w }; }
};

template <>
struct glpix::type_helper<glm::ivec2> { 
    using type = cl_int2; 
    inline static const type cast(const glm::ivec2& val) { return cl_int2 { .x = val.x, .y = val.y }; } 
};

template<> 
struct glpix::type_helper<glm::ivec3> {
    using type = cl_int3; 
    inline static const type cast(const glm::ivec3& val) { return cl_int3 { .x = val.x, .y = val.y, .z = val.z }; }
};

template <>
struct glpix::type_helper<glm::ivec4> { 
    using type = cl_int4; 
    inline static const type cast(const glm::ivec4& val) { return cl_int4 { .x = val.x, .y = val.y, .z = val.z, .w = val.w }; }
};
