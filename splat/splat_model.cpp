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

		new_alpha += m_tree[child].alpha;
		if (new_alpha > 1) new_alpha = 1;	
	}

	bb.rsquared = SQUARE(max_radius);
	bb.alpha = new_alpha;
}

void splat_model::subdivide(sphere& bb) {
  
	int child_count = bb.cend - bb.cbegin;
	//std::cout << " Subdividing: " << child_count << std::endl;

	if (child_count <= BVH_CHILD_STOP_COUNT) 
		return;
  
  	int axis = 0;
	
	/* Get size along axies */
	cl_float3 size = size_along_axies(bb);

  	if (size.y > size.x)  axis = 1;
  	if (size.z > size.s[axis]) axis = 2;

  	float best_split = bb.center.s[axis]; //default to center
	float best_heuristic = INFINITY;

	#if defined(BVH_MC_SAH)
	std::vector<sphere> random_children;
	std::sample(m_tree.begin() + bb.cbegin, m_tree.begin() + bb.cend, std::back_inserter(random_children), 
				std::min(child_count, 10000), std::mt19937 {std::random_device{}()});

	for (int eval_axis = 0; eval_axis < 3; eval_axis++) {
		for (float i = 1; i <= 16; i++) {
		
			float split = bb.center.s[eval_axis] - sqrt(bb.rsquared) + i /16 * size.s[eval_axis];
			sphere lchild = {.rsquared = 0, .count = 0 }, rchild = { .rsquared = 0, .count = 0 };

			for (const auto& child : random_children) {
				if (child.center.s[eval_axis] < split)
					lchild.grow(child);
				else
					rchild.grow(child);
			}
		
			float cost = lchild.rsquared * lchild.count + rchild.rsquared * rchild.count;
			if (cost < best_heuristic && cost > 0) {

				best_heuristic = cost;
				best_split = split;
				axis = eval_axis;
			}
		}
	}
	#else
	for (int eval_axis = 0; eval_axis < 3; eval_axis++) {
		for (float i = 1; i <= 16; i++) {
		
			float split = bb.center.s[eval_axis] - sqrt(bb.rsquared) + i /16 * size.s[eval_axis];
			sphere lchild = {.rsquared = 0, .count = 0 }, rchild = { .rsquared = 0, .count = 0 };
			
			for (int child = bb.cbegin; child < bb.cend; child++) {
				if (m_tree[child].center.s[eval_axis] < split)
					lchild.grow(m_tree[child]);
				else
					rchild.grow(m_tree[child]);
			}

			float cost = lchild.rsquared * lchild.count + rchild.rsquared * rchild.count;
			if (cost < best_heuristic && cost > 0) {

				best_heuristic = cost;
				best_split = split;
				axis = eval_axis;
			}
		}
	}

	#endif

  	int i = bb.cbegin, 
       	j = bb.cend - 1;

  	while (i <= j) {
    	if (m_tree.at(i).center.s[axis] < best_split) ++i;
    	else std::swap(m_tree.at(i), m_tree.at(j--));
  	}

	/* End splitting if plane would not split children */
  	if ((i - bb.cbegin) == 0 || (i - bb.cbegin) == child_count) return; 

	sphere lchild = (sphere) { .cbegin = bb.cbegin, .cend = i };
	sphere rchild = (sphere) { .cbegin = i, .cend = bb.cend };

	fit(lchild);
	fit(rchild);

	m_bvh_build_counter.increment_work();
	subdivide(lchild);
	m_bvh_build_counter.increment_work();
	subdivide(rchild);

	/* Append nodes */
	m_tree.emplace_back(lchild);
	m_tree.emplace_back(rchild);
	
	int cbegin = m_tree.size() - 2,
		cend   = m_tree.size();

	bb.cbegin = cbegin; 
	bb.cend = cbegin + 2;
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