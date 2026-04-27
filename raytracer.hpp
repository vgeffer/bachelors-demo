#pragma once

#include "bvh/primitives.hpp"
#include "lib/glpix.hpp"
#include <glm/glm.hpp>
#include "camera.hpp"

#define EPSILON 0.0001f


struct bvnode { glm::vec3 min, max; uint l, first, count; };
struct alignas(sizeof(glm::vec4)) fdata { glm::vec4 p0, p1, p2, pos; glm::mat4x4 view; float f; };

/* for demo - AABB node */
class raytracer : public glpix {
  public:
    raytracer();

  private:
    bool create() override;   
    bool update(float delta) override;

  private:
    inline float randf() { return (static_cast<float>(rand() % 65536) / 65536.0f); }


    float intersect_triangle(const ray& r, const tri& t);
    bool intersect_AABB(const ray& r, const glm::vec3& min, const glm::vec3& max);

    void build_bvh();
    void update_bounds(bvnode& node);
    void subdivide(bvnode& node);
    void load_obj(const char* path);

    void draw_bvh(ray& r, const bvnode& root);

  private:
    camera m_c;

    std::vector<tri> triangles;
    uint* tri_indices = nullptr;
    bvnode* nodes = nullptr;
    fdata m_data;

    //bvh<aabb, tri> m_bvh;

    uint root = 0, used = 1;
};

template <typename ...T>
glm::vec3 vmin(T... args) {
  
  glm::vec3 min = glm::vec3(INFINITY);
  for (auto arg : {args...}) {
    if (arg.x < min.x) min.x = arg.x;
    if (arg.y < min.y) min.y = arg.y;
    if (arg.z < min.z) min.z = arg.z;
  }
  return min;
}

template <typename ...T>
glm::vec3 vmax(T... args) {

  glm::vec3 max = glm::vec3(-INFINITY);
  for (auto arg : {args...}) {
    if (arg.x > max.x) max.x = arg.x;
    if (arg.y > max.y) max.y = arg.y;
    if (arg.z > max.z) max.z = arg.z;
  }
  return max;
}

