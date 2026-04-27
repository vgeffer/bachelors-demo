///
/// @file camera.hpp
/// @author geffevil
///
#pragma once
#include <glm/glm.hpp>


class camera {  
  public:
    /// @brief Camera constructor
    ///
    /// Constructs new camera from given params
    ///
    /// @param parent Parent node of this component
    /// @param fov Camera's Field of View
    /// @param near Camera's near clipping plane
    /// @param far Camera's far clipping plane
    /// @param main Wether or not to activate the camera when it enters the scene
    /// @see scene::node_component
    camera() = default;
    camera(glm::vec4 pos, float fov, float near);

    glm::mat4x4 view() const;

    const glm::vec4 up() const;
    const glm::vec4 forward() const;
    const std::tuple<glm::vec4, glm::vec4, glm::vec4> view_vectors(float aspect);

    /* Setting projection parameters also updates stored matrices */
    inline const glm::vec4& pos() const { return m_pos; }
    const glm::vec4 pos(const glm::vec4& p) { return m_pos = p; } 

    inline const glm::vec4& rot() const { return m_rot; }
    const glm::vec4 rot(const glm::vec4& r) { return m_rot = r; } 
    
    inline float fov() const { return m_fov; }
    float fov(const float& fov);
            
    inline float near() const { return m_near; }
    float near(const float& near);

    private:
      glm::vec4 m_pos;
      glm::vec4 m_rot;
      float m_fov, m_near;
};
