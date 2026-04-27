#include "raytracer.hpp"
#include "lib/glpix.hpp"
#include <iostream>

using namespace glm; 
using namespace std;


const char* gpu_pixel_func = R"(


  	layout (std140) uniform frame_data {
	  	vec4 p0, p1, p2;
		vec4 pos;
		mat4x4 view;
		float f;
  	};

  	struct ray {
	  	vec4 origin, dir; 
  	};


  	float sphereSDF(vec3 p, vec3 c, float sqradius) {
	  	return length(p - c) - sqradius;
  	}

  	float boxSDF(vec3 p, vec3 c, vec3 s) {
	  	vec3 v = abs(p - c) - s;
	  	return max(max(v.x, v.y), v.z);
  	}

  	float intersectSDF(float distA, float distB) {
	  	return max(distA, distB);
  	}

  	float unionSDF(float distA, float distB) {
	  	return min(distA, distB);
  	}

  	float differenceSDF(float distA, float distB) {
	  	return max(distA, -distB);
  	}

  	float sceneSDF(vec3 p) {
	   	return intersectSDF(
			boxSDF(p, vec3(0, 0, 0), vec3(1, 1, 1)),
	   		sphereSDF(p + vec3(0, sin(time), 0), vec3(0, 0, 0), 1.4)
		);
  	}

  	vec4 pixel() {
		vec4 px = p0 + (p1 - p0) * uv.x
					 + (p2 - p0) * uv.y;

		ray r = ray(pos, normalize(px * view));
		float depth = 0;

	  	for (float step = 0; step < 256; step++) {
		  	vec3 point = r.origin.xyz + depth * r.dir.xyz;
		  	float dist = sceneSDF(point);

		  	if (dist < 0.1) 
			  	return vec4(0, 1 - (step / 255), 0, 1);

		  	depth += dist;
		  	if (depth > 1000)
			  	break;
	  	}

	  	return vec4(0, 0, 0, 1);
  	}
)";

raytracer::raytracer() : glpix("RT-DEMO", 1600, 1200, false, gpu_pixel_func, sizeof(fdata)) {}


bool raytracer::create() {
  
	m_c = camera(vec4(0, 0, -20, 1), glm::radians(90.0f), 5);
	return true;
}

float counter = 0;
int frames = 0;

bool raytracer::update(float delta) {

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

    auto [p0, p1, p2] = m_c.view_vectors(static_cast<float>(win_width()) / static_cast<float>(win_height()));
  	m_data.p0 = p0;
	m_data.p1 = p1;
	m_data.p2 = p2;
	m_data.pos = m_c.pos();
	m_data.view = m_c.view();
	m_data.f = sin(time());

  	frame_data(&m_data);
  	return true;
}


int main() {
  raytracer rt;
  rt.start();

  return 0;
}
