#include "splat_model.hpp"
#include "include/rtutils.h"
#include <cmath>
#include <stdexcept>

#define SQUARE(x) ((x) * (x))

void splat_model::refit() {

	/* For now: Do a simple post-order traversal */

	throw std::logic_error("Not Implemented!");
}


void combine_nodes(int count, int* indices) {

    if (count > BVH_NUM_CHILDREN)
        throw std::runtime_error("More than BVH_NUM_CHILDREN passed to combine_nodes");

	float alpha = 0;
	/* Clamp alpha */
	if (alpha > 1)
		alpha = 1;

	fit()

	/* Assign children */
    int children[BVH_NUM_CHILDREN];

}

void fit(sphere& bb) {

	int child_count = 0;

	/* Recompute origin */
	for (int child = bb.children[child_count]; child >= 0 && child_count < BVH_NUM_CHILDREN; child_count++) {
		bb.center.x = nodes[child].center.x;
		bb.center.y = nodes[child].center.y;
		bb.center.z = nodes[child].center.z;
		bb.center.w = nodes[child].center.w;		
	}

	if (child_count <= 0) return; /* Ear*/

	bb.center.x /= child_count; 
	bb.center.y /= child_count; 
	bb.center.z /= child_count; 
	bb.center.w /= child_count; 

	/* Compute the radius */
	float max_radius = -INFINITY; /* Center + radius */
	for (int i = 0, child = bb.children[i]; child >= 0 && i <= child_count; i++) {

		/* Calculate */
		float new_radius = sqrt(SQUARE(nodes[child].x - bb.center.x) + 
								SQUARE(noded[child].y - bb.center.y) + 
								SQUARE(nodes[child].z - bb.center.z)) + sqrt(nodes[child].rsquared);
		max_radius = std::max(max_radius, new_radius);
	}

	bb.rsquared = SQUARE(max_radius);
}

// Return the bounding sphere of the leaves in positions [begin..end)
QTree_Node *QTree::rec_build_tree(int begin, int end)
{
#ifdef DEBUG
	if (begin == end) {
		fprintf(stderr, "Yikes! begin == end in BuildLeaves()\n");
		return NULL;
	}
#endif

	if (end - begin <= BVH_NUM_CHILDREN)
		return CombineLeaves(begin, end);

	// Recursive case
	int middle = Partition(begin, end);

	if (middle - begin <= 4) {
		if (end - middle <= 4) {
			return CombineNodes(CombineLeaves(begin, middle),
					    CombineLeaves(middle, end));
		} else {
			int m2 = Partition(middle, end);
			return CombineNodes(CombineLeaves(begin, middle),
					    BuildLeaves(middle, m2),
					    BuildLeaves(m2, end));
		}
	} else {
		if (end - middle <= 4) {
			int m1 = Partition(begin, middle);
			return CombineNodes(BuildLeaves(begin, m1),
					    BuildLeaves(m1, middle),
					    CombineLeaves(middle, end));
		} else {
			int m1 = Partition(begin, middle);
			int m2 = Partition(middle, end);
			return CombineNodes(BuildLeaves(begin, m1),
					    BuildLeaves(m1, middle),
					    BuildLeaves(middle, m2),
					    BuildLeaves(m2, end));
		}
	}
}


// Do partitioning on the range [begin..end) of thetree.  Returns index on
// which we partition (i.e. partitions are [begin..middle) and [middle..end) )
int QTree::Partition(int begin, int end)
{
#ifdef DEBUG
	if (end - begin <= 4)
		fprintf(stderr, "end - begin <= 4 in Partition()\n");
#endif

	float xmin, xmax, ymin, ymax, zmin, zmax;
	FindBBox(begin, end, xmin, xmax, ymin, ymax, zmin, zmax);
	float dx = xmax-xmin, dy = ymax-ymin, dz = zmax-zmin;
	if (!dx && !dy && !dz) {
#ifdef DEBUG
		fprintf(stderr, "dx, dy, dz all zero in Partition()\n");
#endif
		return (begin+end)/2;
	}


	// Split along longest axis
	int splitaxis;
	float axis_min, axis_max;
	if (dx > dy) {
		if (dx > dz) {
			splitaxis = 0;
			axis_min = xmin;  axis_max = xmax;
		} else {
			splitaxis = 2;
			axis_min = zmin;  axis_max = zmax;
		}
	} else {
		if (dy > dz) {
			splitaxis = 1;
			axis_min = ymin;  axis_max = ymax;
		} else {
			splitaxis = 2;
			axis_min = zmin;  axis_max = zmax;
		}
	}

	float splitval = 0.5f * (axis_min + axis_max);
	int middle = PartitionAlongAxis(begin, end, splitaxis, splitval);


	// If we're down to a small number of nodes, don't try to do
	// anything fancy, just split along the middle
	if (end - begin <= 64)
		return middle;


	// Otherwise, try a few more candidate splits
	splitval = 0.6f * axis_min + 0.4f * axis_max;
	int mid1 = PartitionAlongAxis(begin, middle, splitaxis, splitval);

	splitval = 0.4f * axis_min + 0.6f * axis_max;
	int mid2 = PartitionAlongAxis(middle, end, splitaxis, splitval);

	float xmin1, xmax1, ymin1, ymax1, zmin1, zmax1;
	float xmin2, xmax2, ymin2, ymax2, zmin2, zmax2;
	float xmin3, xmax3, ymin3, ymax3, zmin3, zmax3;
	float xmin4, xmax4, ymin4, ymax4, zmin4, zmax4;
	FindBBox(begin,  mid1,   xmin1, xmax1, ymin1, ymax1, zmin1, zmax1);
	FindBBox(mid1,   middle, xmin2, xmax2, ymin2, ymax2, zmin2, zmax2);
	FindBBox(middle, mid2,   xmin3, xmax3, ymin3, ymax3, zmin3, zmax3);
	FindBBox(mid2,   end,    xmin4, xmax4, ymin4, ymax4, zmin4, zmax4);

	float vol1 = (xmax1-xmin1) * (ymax1-ymin1) * (zmax1-zmin1) +
		     (max(max(xmax2, xmax3), xmax4) - min(min(xmin2, xmin3), xmin4)) *
		     (max(max(ymax2, ymax3), ymax4) - min(min(ymin2, ymin3), ymin4)) *
		     (max(max(zmax2, zmax3), zmax4) - min(min(zmin2, zmin3), zmin4));

	float vol2 = (max(xmax1, xmax2) - min(xmin1, xmin2)) *
		     (max(ymax1, ymax2) - min(ymin1, ymin2)) *
		     (max(zmax1, zmax2) - min(zmin1, zmin2))  +
		     (max(xmax3, xmax4) - min(xmin3, xmin4)) *
		     (max(ymax3, ymax4) - min(ymin3, ymin4)) *
		     (max(zmax3, zmax4) - min(zmin3, zmin4));
	// An even split is desirable, so we give this a slight bonus
	vol2 *= 0.95f;

	float vol3 = (max(max(xmax1, xmax2), xmax3) - min(min(xmin1, xmin2), xmin3)) *
		     (max(max(ymax1, ymax2), ymax3) - min(min(ymin1, ymin2), ymin3)) *
		     (max(max(zmax1, zmax2), zmax3) - min(min(zmin1, zmin2), zmin3))  +
		     (xmax4-xmin4) * (ymax4-ymin4) * (zmax4-zmin4);

	if (vol1 < vol3) {
		if (vol1 < vol2) return mid1;
	} else {
		if (vol3 < vol2) return mid2;
	}
	return middle;
}


// Partition the range [begin..end) of thetree along the given axis, at the
// given value.  Returns index on which we partition.
int QTree::PartitionAlongAxis(int begin, int end, int axis, float splitval)
{
	int left = begin, right = end-1;

	while (1) {
		// March the "left" pointer to the right
		while (leafptr[left]->pos[axis] < splitval)
			left++;

		// March the "right" pointer to the left
		while (leafptr[right]->pos[axis] >= splitval)
			right--;

		// If the pointers have crossed, we're done
		if (right < left) {
#ifdef DEBUG
			if (left == begin) {
				fprintf(stderr, "Yikes! left == begin in PartitionAlongAxis()\n");
				left++;
			} else if (left == end) {
				fprintf(stderr, "Yikes! left == end in PartitionAlongAxis()\n");
				left--;
			}
#endif
			return left;
		}

		// Else, swap and repeat
		swap(leafptr[left], leafptr[right]);
	}
}

