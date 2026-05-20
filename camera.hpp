///
/// @file camera.hpp
/// @author geffevil
///
#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include "lib/glpix.hpp"
#include "math.hpp"
#include <glm/gtx/string_cast.hpp>

class camera {  
  	public:
    	struct __attribute__ ((packed)) gpu_struct {
      
    		cl_float4 p0, p1, p2, pos;
			mat4f view;
		};

  	public:
		camera() = default;
    	camera(glm::vec4 pos, float fov, float near, float aspect);

    	glm::mat4x4 view() const;

    	const glm::vec4 up() const;
    	const glm::vec4 forward() const;
    	const std::tuple<glm::vec4, glm::vec4, glm::vec4> view_vectors() const;

    	/* Setting projection parameters also updates stored matrices */
    	inline const glm::vec4& pos() const { return m_pos; }
    	const glm::vec4 pos(const glm::vec4& p) { return m_pos = p; } 

    	inline const glm::vec4& rot() const { return m_rot; }
    	const glm::vec4 rot(const glm::vec4& r) { return m_rot = r; } 
		const gpu_struct togpu() {

        	const auto [p0, p1, p2] = view_vectors();
        	const glm::mat4 viewm = view();
			//std::cout  << glm::to_string(viewm) << std::endl;
        camera::gpu_struct out = {
            .p0  = cl_float4 { p0.x,  p0.y,  p0.z,  p0.w },
            .p1  = cl_float4 { p1.x,  p1.y,  p1.z,  p1.w },
            .p2  = cl_float4 { p2.x,  p2.y,  p2.z,  p2.w },
            .pos = cl_float4 { pos().x, pos().y, pos().z, pos().w}, 
            .view = {
                .columns = { 
                cl_float4 { viewm[0].x, viewm[0].y, viewm[0].z, viewm[0].w },
                cl_float4 { viewm[1].x, viewm[1].y, viewm[1].z, viewm[1].w },
                cl_float4 { viewm[2].x, viewm[2].y, viewm[2].z, viewm[2].w },
                cl_float4 { viewm[3].x, viewm[3].y, viewm[3].z, viewm[3].w },
                }
            }
        };

        return out;
    }

    	inline float fov() const { return m_fov; }
    	float fov(const float& fov);
            
    	inline float near() const { return m_near; }
    	float near(const float& near);

    private:
      	glm::vec4 m_pos;
      	glm::vec4 m_rot;
      	float m_fov, m_near;
		float m_aspect;
};
