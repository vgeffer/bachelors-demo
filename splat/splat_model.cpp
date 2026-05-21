#include "splat_model.hpp"
#include "include/rtutils.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

#define SQUARE(x) ((x) * (x))

void splat_model::refit() {

	/* For now: Do a simple post-order traversal */

	throw std::logic_error("Not Implemented!");
}


void splat_model::fit(sphere& bb) {

	int child_count = bb.cend - bb.cbegin;
	if (child_count <= 0) return; /* End early as there is nothing to fit */

	/* Recompute origin */
	for (int child = bb.cbegin; child < bb.cend; child++) {
		bb.center.x += m_tree[child].center.x;
		bb.center.y += m_tree[child].center.y;
		bb.center.z += m_tree[child].center.z;
		bb.center.w += m_tree[child].center.w;		
	}

	bb.center.x /= child_count; 
	bb.center.y /= child_count; 
	bb.center.z /= child_count; 
	bb.center.w /= child_count; 

	/* Compute the radius */
	float max_radius = -INFINITY; /* Center + radius */
	float new_alpha = 0;
	for (int child = bb.cbegin; child < bb.cend; child++) {

		/* Calculate */
		float new_radius = sqrt(SQUARE(m_tree[child].center.x - bb.center.x) + 
								SQUARE(m_tree[child].center.y - bb.center.y) + 
								SQUARE(m_tree[child].center.z - bb.center.z)) + sqrt(m_tree[child].rsquared);
		max_radius = std::max(max_radius, new_radius);

		new_alpha += m_tree[child].color.w;
		if (new_alpha > 1) new_alpha = 1;	
	}

	bb.rsquared = SQUARE(max_radius);
	bb.color = (float4) { .w = new_alpha };
}

void splat_model::subdivide(sphere& bb) {
  
	int child_count = bb.cend - bb.cbegin;
	//std::cout << " Subdividing: " << child_count << std::endl;

	if (child_count <= BVH_CHILD_STOP_COUNT) 
		return;
  
  	int axis = 0;
	
	/* Get size along axies */
	cl_float3 size = size_along_axies(bb);

	//std::cout << "	Sizes along axies: " << size.x << " " << size.y << " " << size.z << std::endl;

  	if (size.y > size.x)  axis = 1;
  	if (size.z > size.s[axis]) axis = 2;

  	float split = bb.center.s[axis];// - sqrt(bb.rsquared) + size.s[axis] * 0.5f;
	//std::cout << "	Split axis: " << axis << ", split plane: " << split << std::endl;


  	int i = bb.cbegin, 
       	j = bb.cend - 1;

  	while (i <= j) {
    	if (m_tree[i].center.s[axis] < split) ++i;
    	else std::swap(m_tree[i], m_tree[j--]);
  	}

	//std::cout << "	Children left: " << i - bb.cbegin << ", Children right: " << bb.cend - i << std::endl;


	/* End splitting if plane would not split children */
  	if ((i - bb.cbegin) == 0 || (i - bb.cbegin) == child_count) return; 



	/* Fit first two */
	m_tree.emplace_back((sphere) { .cbegin = bb.cbegin, .cend = i });
	m_tree.emplace_back((sphere) { .cbegin = i, .cend = bb.cend });
	
	int cbegin = m_tree.size() - 2,
		cend   = m_tree.size();

	fit(m_tree[cbegin]);
	fit(m_tree[cbegin + 1]);

	subdivide(m_tree[cbegin]);
	subdivide(m_tree[cbegin + 1]);
	
	bb.cbegin = cbegin; bb.cend = cbegin + 2;
}

cl_float3 splat_model::size_along_axies(const sphere& bb) {

	cl_float3 min = (cl_float3) { .x = 0, .y = 0, .z = 0};
	cl_float3 max = (cl_float3) { .x = 0, .y = 0, .z = 0 };

	for (int child = bb.cbegin; child < bb.cend; child++) {

		float sqradius = sqrt(m_tree[child].rsquared);

		min.x = std::min(min.x, m_tree[child].center.x - sqradius);
		max.x = std::max(max.x, m_tree[child].center.x + sqradius);

		min.y = std::min(min.y, m_tree[child].center.y - sqradius);
		max.y = std::max(max.y, m_tree[child].center.y + sqradius);

		min.z = std::min(min.z, m_tree[child].center.z - sqradius);
		max.z = std::max(max.z, m_tree[child].center.z + sqradius);
	}

	return (float3) { .x = fabs(max.x - min.x), .y = fabs(max.y - min.y), .z = fabs(max.z - min.z) };
}