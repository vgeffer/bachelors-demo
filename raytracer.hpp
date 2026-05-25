#pragma once

#include "lib/glpix.hpp"
#include <glm/glm.hpp>
#include "camera.hpp"
#include "lib/tiny_bvh/tiny_bvh.h"
#include "splat/include/rtutils.h"
#include "splat/splat_model.hpp"

#define EPSILON 0.001f


/* for demo - AABB node */
class raytracer : public glpix {
    public:
      raytracer(int benchmark_length, const char* model);

    private:
        bool create() override;   
        bool update(float delta) override;

    private:
        inline float randf() { return (static_cast<float>(rand() % 65536) / 65536.0f); }
    
    private:
        camera m_c;
        glpix::kernel<cl_mem, cl_mem, cl_mem, int, float, int, int> m_trace_kernel;
        glpix::buffer<camera::gpu_struct> m_camera_buffer;
        glpix::buffer<sphere> m_bvh;

        splat_model m_model;
        int m_benchmark_length;
        const std::string m_model_path;

        uint root = 0, used = 1;
};

