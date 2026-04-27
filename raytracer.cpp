#include "raytracer.hpp"
#include "lib/glpix.hpp"
#include <cmath>
#include <cstdlib>
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

  	float sceneSDF(vec3 p, float time) {
	   return sphereSDF(p + vec3(0, sin(time), 0), vec3(0, 0, 0), 1.4);
  	}

  	vec4 pixel() {
		/*vec4 px = p0 + (p1 - p0) * uv.x
					 + (p2 - p0) * uv.y;

		ray r = ray(pos, px * view);
		float depth = 0;

	  	for (float step = 0; step < 256; step++) {
		  	vec3 point = r.origin.xyz + depth * r.dir.xyz;
		  	float dist = sceneSDF(point, time);

		  	if (dist < 0.1) 
			  	return vec4(step / 255, 0, 0, 1);

		  	depth += dist;
		  	if (depth > 1000)
			  	break;
	  	}*/

	  	return vec4(f, 0, 0, 1);
  	}
)";

raytracer::raytracer() : glpix("RT-DEMO", 800, 600, false, gpu_pixel_func, sizeof(fdata)) {}


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


void raytracer::load_obj(const char* path) {

  /*triangles.clear();
  if (tri_indices != nullptr)
	delete[] tri_indices;

  ifstream obj_in = ifstream(path, std::ios::in);
  if (!obj_in)
	throw runtime_error("Cannot open " + string(path));
		
  std::string line;
  vector<glm::vec4> vertices;

  while (std::getline(obj_in, line))
  {
	if (line.substr(0,2)=="v ") {
	  
	  istringstream v(line.substr(2));
	  float x,y,z;
	  v >> x; v >> y; v >> z;

	  vertices.emplace_back(x, y, z);
	}
	
	else if(line.substr(0,2)=="f "){
	  int a, b, c;
	  const char* chh = line.c_str();
	  sscanf (chh, "f %i %i %i", &a, &b, &c);
	  
	  tri t = {
		vertices[a - 1], vertices[b - 1], vertices[c - 1],
		(vertices[a - 1] + vertices[b - 1] + vertices[c - 1]) / 3.0f,
		glpix::color(static_cast<uint32_t>(rand()) | 0xFF)
	  };
	  triangles.push_back(t);
	}
  }

  tri_indices = new uint[triangles.size()];
  for (int i = 0; i < triangles.size(); i++)
	tri_indices[i] = i;*/
}

int main() {
  raytracer rt;
  rt.start();

  return 0;
}
