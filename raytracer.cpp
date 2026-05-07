#include "lib/glpix.hpp"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
#include "raytracer.hpp"

#include "lib/tiny_bvh/tiny_bvh.h"

#include <OpenVDB/openvdb.h>

using namespace glm; 
using namespace std;

raytracer::raytracer() : glpix("RT-DEMO", 1280, 720, false) {}

bool raytracer::create() {
  
    m_test_kernel = create_kernel<cl_mem, float, int, int>("kernels/test.cl", "test", gpu_screen_buffer(), 1.0f, win_width(), win_height());

    openvdb::initialize();
    load_obj("test.obj");;

    //tinybvh::BVH_GPU bvh;
    //bvh.Build((const tinybvh::bvhvec4*) vertices.data(), (const uint32_t*) indices.data(), indices.size() / 3);

    m_c = camera(vec4(0, 0, -20, 1), glm::radians(90.0f), 5);
    //std::cout << "Model has " << triangles.size() << " triangles and " << used << " BVH nodes" << std::endl;

    openvdb::io::File file = openvdb::io::File("cloud.vdb");
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

    openvdb::FloatTree grid = openvdb::gridPtrCast<openvdb::FloatGrid>(base_ptr)->tree();

    draw_kernel(m_test_kernel);
    std::cout << grid.activeLeafVoxelCount() << std::endl;
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

    m_test_kernel.set_arg<1>(time() * 0.2f);

    return true;
}

void raytracer::load_obj(const char* path) {

    vertices.clear();
    indices.clear();
    
    ifstream obj_in = ifstream(path, std::ios::in);
    if (!obj_in)
        throw runtime_error("Cannot open " + string(path));
        
    std::string line;
    vector<glm::vec3> vertices;

    while (std::getline(obj_in, line))
    {
        if (line.substr(0,2)=="v ") {
      
            istringstream v(line.substr(2));
            float x,y,z;
            v >> x; v >> y; v >> z;

            vertices.emplace_back(x, y, z);
        }
    
        else if(line.substr(0,2)=="f "){
            
            istringstream v(line.substr(2));
            uint32_t a, b, c;
            v >> a; v >> b; v >> c;
            
            indices.emplace_back(a);
            indices.emplace_back(b);
            indices.emplace_back(c);
        }
    }
}


int main() {
    raytracer rt;
	rt.start();

	return 0;
}
