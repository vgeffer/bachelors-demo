#include "camera.hpp"
#include "lib/glpix.hpp"
#include <cstring>
#include <glm/ext/vector_float4.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <ostream>
#include <string>
#include "raytracer.hpp"

#include "lib/tiny_bvh/tiny_bvh.h"
#include "splat/include/rtutils.h"
#include "splat/splat_model.hpp"

#if defined(PLATFORM_UNIX)
    #include <openvdb/openvdb.h>
#elif defined(PLATFORM_MACOS)
    #include <OpenVDB/openvdb.h>
    #include <OpenVDB/tools/NodeVisitor.h>
#endif

using namespace glm; 
using namespace std;

raytracer::raytracer(int benchmark_length, const char* model) 
    : glpix("RT-DEMO", 1920, 1080, false), m_benchmark_length(benchmark_length), m_model_path(model) {}

bool raytracer::create() {

    /* Init OpenVDB */
    openvdb::initialize();

    /* Init Camera */
    m_c = camera(vec4(0, 0, -800, 1), glm::radians(70.0f), 5, static_cast<float>(win_width()) / static_cast<float>(win_height()));
    m_c.rot(vec4(0, glm::radians(180.0f), 0, 0));

    /* Load OpenVDB File */
    openvdb::io::File file = openvdb::io::File(m_model_path);
    file.open();

    openvdb::GridBase::Ptr base_ptr;
    int grid_idx = 0;

    for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter)
    {
        std::cerr << "Found Grid [" << grid_idx++ << "]: " << nameIter.gridName() << std::endl;

        // Read in only the grid we are interested in.
        if (nameIter.gridName() == "density")
            base_ptr = file.readGrid(nameIter.gridName());
        else
            std::cout << "skipping grid " << nameIter.gridName() << std::endl;
        
    }
    file.close();
    m_model = splat_model(openvdb::gridPtrCast<openvdb::FloatGrid>(base_ptr)->tree());

    /* Setup kernel */
    camera::gpu_struct cam = m_c.togpu();
    m_camera_buffer = create_buffer<camera::gpu_struct>(4, buffer<float>::usage::RW, &cam);

    const auto tree = m_model.tree();
    m_bvh = create_buffer<sphere>(tree.size(), buffer<sphere>::usage::RD_ONLY, tree.data());

    m_trace_kernel = create_kernel<cl_mem, cl_mem, cl_mem, int, float, int, int>(
        "splat/kernels/raytracer.cl", 
        "raymarch", gpu_screen_buffer(), m_camera_buffer, m_bvh, tree.size(), 1.0f, win_width(), win_height());


    if (m_benchmark_length > 0) {
        std::cerr << "Benchmarking mode turned on. Application will output FPS with timestamp in a CSV format to the stdout.\nApplication will also auto-close after "
                  <<  m_benchmark_length << " minutes!" << std::endl;
        std::cout << "TIME,FPS" << std::endl;
    }

    set_draw_kernel(m_trace_kernel);
    return true;
}

float counter = 0;
int frames = 0;

bool raytracer::update(float delta) {

    clear(glpix::BLACK);

    if (m_benchmark_length > 0) {
        ++frames;
        counter += delta;
        if (counter >= 5.0f) {
            std::cout << time() << "," << frames / counter << std::endl;
            counter = 0.0f;
            frames = 0;
        }
        
        /* Kill after 10 minutes */
        if (time() > m_benchmark_length * 60) return false;
    }

    if (key_held(glpix::key::W)) { 
        glm::vec4 c_pos = m_c.pos(); 
        m_c.pos(c_pos - m_c.forward() * 10.0f * delta);
    } 
  
    if (key_held(glpix::key::S)) { 
        glm::vec4 c_pos = m_c.pos();
        m_c.pos(c_pos + m_c.forward() * 10.0f * delta);
    } 

    if (key_held(glpix::key::A)) { 
        glm::vec4 c_rot = m_c.rot();
        c_rot.y += 1.68f * delta;
        m_c.rot(c_rot);
    } 
  
    if (key_held(glpix::key::D)) { 
        glm::vec4 c_rot = m_c.rot();
        c_rot.y -= 1.68f * delta;
        m_c.rot(c_rot);
    }

    camera::gpu_struct cam = m_c.togpu();
    write_buffer(m_camera_buffer, 4, &cam).await();
    auto [p0, p1, p2] = m_c.view_vectors();
    
    m_trace_kernel.set_arg<3>(time() * 64);

    return true;
}


/* Argument parsing macros */
#define ARG_CHECK(lname, sname) if (strncmp(argv[i], lname, sizeof(lname)) == 0 || strncmp(argv[i], sname, sizeof(sname)) == 0)
#define NEXT_ARG_EX_CHECK if (i + 1 >= argc) { std::cerr << "Argument " << argv[i] << " expects aditional value!" << std::endl; return -1; }

#define BOOL_FLAG(lname, sname, var) ARG_CHECK(lname, sname) { var = true; continue; }
#define INT_VALUE(lname, sname, var) ARG_CHECK(lname, sname) { NEXT_ARG_EX_CHECK var = std::atoi(argv[++i]); continue;}

int main(int argc, char** argv) {

    const char* model_file = "";
    int benchmark_length = -1;

    if (argc < 2) {
        std::cerr << "No model file provided!\nUsage: raytracer [model] (flags)" << std::endl;
        return -1;
    }

    model_file = argv[1];
    for (int i = 2; i < argc; i++) {

        /* Parse further args */
        INT_VALUE("--benchmark", "-b", benchmark_length);
    }

    raytracer rt(benchmark_length, model_file);
	rt.start();

	return 0;
}
