#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

using namespace glm;

constexpr vec4 UP = vec4(0, -1, 0, 0);

     
camera::camera(glm::vec4 pos, float fov, float near) 
    : m_pos(pos), m_fov(fov), m_near(near), m_rot(vec4(0, 0, 0, 0)) {}

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

const std::tuple<vec4, vec4, vec4> camera::view_vectors(float aspect) {

  float tan_fov_half = tan(m_fov / 2);

  vec4 p0 = vec4(
    -m_near * aspect * tan_fov_half,
     m_near          * tan_fov_half,
     m_near, 0                         
  );

  vec4 p1 = vec4(
     m_near * aspect * tan_fov_half,
     m_near          * tan_fov_half,
     m_near, 0                         
  );

  vec4 p2 = vec4(
    -m_near * aspect * tan_fov_half,
    -m_near          * tan_fov_half,
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
