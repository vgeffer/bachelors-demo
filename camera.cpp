#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera.hpp"
#include "lib/glpix.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

using namespace glm;

constexpr vec4 UP = vec4(0, -1, 0, 0);

     
camera::camera(glm::vec4 pos, float fov, float near, float aspect) 
    : m_pos(pos), m_fov(fov), m_near(near), m_rot(vec4(0, 0, 0, 0)), m_aspect(aspect) {}

mat4x4 camera::view() const {
    return lookAt(vec3(m_pos), vec3(m_pos + forward()), vec3(up()));
}

const vec4 camera::up() const {
    return normalize(UP);
}

const vec4 camera::forward() const {
  return vec4(
      sin(m_rot.y) * cos(m_rot.x),
      sin(m_rot.y) * sin(m_rot.x),
      cos(m_rot.y), 0
  );
}

const std::tuple<vec4, vec4, vec4> camera::view_vectors() const {

  float tan_fov_half = tan(m_fov / 2);

  vec4 p0 = vec4(
    -m_near * m_aspect * tan_fov_half,
     m_near            * tan_fov_half,
     m_near, 0                         
  );

  vec4 p1 = vec4(
     m_near * m_aspect * tan_fov_half,
     m_near            * tan_fov_half,
     m_near, 0                         
  );

  vec4 p2 = vec4(
    -m_near * m_aspect * tan_fov_half,
    -m_near            * tan_fov_half,
     m_near, 0                        
  );

  return std::make_tuple(p0, p1, p2);
}

/* Upload projection matrix to OpenGL only when dirty */
float camera::fov(const float& fov) {
    return m_fov = fov;
}

float camera::near(const float& near) {
    return m_near = near;
}

template<>
struct glpix::type_helper<camera> { 
    using type = camera::gpu_struct; 
    static inline const type cast(const camera& val) { 

        const auto [p0, p1, p2] = val.view_vectors();
        const vec4 pos = val.pos();
        const mat4 view = val.view();

        camera::gpu_struct out = {
            .p0  = cl_float4 { p0.x,  p0.y,  p0.z,  p0.w },
            .p1  = cl_float4 { p1.x,  p1.y,  p1.z,  p1.w },
            .p2  = cl_float4 { p2.x,  p2.y,  p2.z,  p2.w },
            .pos = cl_float4 { pos.x, pos.y, pos.z, pos.w}, 
            .view = {
                .columns = { 
                cl_float4 { view[0].x, view[0].y, view[0].z, view[0].w },
                cl_float4 { view[1].x, view[1].y, view[1].z, view[1].w },
                cl_float4 { view[2].x, view[2].y, view[2].z, view[2].w },
                cl_float4 { view[3].x, view[3].y, view[3].z, view[3].w },
                }
            }
        };

        return out;
    }
}; 