#include "camera.hpp"
#include "lib/glpix.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
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

raytracer::raytracer() : glpix("RT-DEMO", 1280, 720, false) {}

bool raytracer::create() {

    /* Init OpenVDB */
    openvdb::initialize();

    /* Init Camera */
    m_c = camera(vec4(0, 0, -20, 1), glm::radians(90.0f), 5, static_cast<float>(win_width()) / static_cast<float>(win_height()));

    /* Load OpenVDB File */
    openvdb::io::File file = openvdb::io::File("cloud-lores.vdb");
    file.open();

    openvdb::GridBase::Ptr base_ptr;
    int grid_idx = 0;

    for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter)
    {
        std::cout << "Found Grid [" << grid_idx++ << "]: " << nameIter.gridName() << std::endl;

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

    m_test_kernel = create_kernel<cl_mem, cl_mem, cl_mem, int, float, int, int>(
        "splat/kernels/raytracer.cl", 
        "raymarch", gpu_screen_buffer(), m_camera_buffer, m_bvh, tree.size(), 1.0f, win_width(), win_height());


    set_draw_kernel(m_test_kernel);
    return true;
}

float counter = 0;
int frames = 0;

bool raytracer::update(float delta) {

    clear(glpix::BLACK);

    ++frames;
    counter += delta;
    if (counter >= 10.0f) {
        std::cerr << "Average FPS: " << frames / counter << std::endl;
        counter = 0.0f;
        frames = 0;
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
    
    m_test_kernel.set_arg<4>(time() * 64);

    return true;
}

int main() {
    raytracer rt;
	rt.start();

	return 0;
}
