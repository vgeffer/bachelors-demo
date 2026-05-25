#include "kernels/include/debug.cl"
#include "splat/include/rtutils.h"

/* Stack wrappers for code readability */
#define MAKE_STACK(type, size) type stack[(size)]; int stack_ptr = 0; const int max_stack_size = size;
#define STACK_POP() stack[--stack_ptr];
#define STACK_PUSH(item) stack[stack_ptr++] = (item);
#define STACK_IS_EMPTY() (stack_ptr <= 0)
#define STACK_IS_FULL() (stack_ptr >= max_stack_size)
#define STACK_PTR stack_ptr

typedef struct __attribute__ ((packed)) _camera {
	float4 p0, p1, p2, pos;
	mat4f view;
} camera;

/* Fast Ray-Sphere algorithm from RTR */
float blas_trace(__global const sphere* tree, const ray r, const int sid) {

	const sphere s = tree[sid];

	float4 l = s.center - r.origin;
	float b = dot(l, r.dir),
		  c = dot(l, l) - s.rsquared,
		  h = b * b - c;

	if (h < 0) return INFINITY;
	return -b - sign(c) * sqrt(h);
}

/* Borrowed from https://stackoverflow.com/questions/23975555/how-to-calculate-a-ray-plane-intersection*/
hit splat_trace(__global const sphere* tree, const ray r, const int sid) {

	const sphere s = tree[sid];

	/* Splats will always be parallel to the screen */
	float4 normal = r.origin - s.center; 

	/* Ray-Plane intersection */
	float denom = dot(r.dir, normal);	
    float t = dot(s.center - r.origin, normal) / denom;
    if (t < RT_EPSILON) return (hit) { .primitive = -1 } ; // you might want to allow an epsilon here too

	/* Check if intersection point is within the circle */
	float4 p = r.origin + t * r.dir;
	if (dot(s.center - p, s.center - p) > s.rsquared) return (hit) { .primitive = -1 };
	return (hit) { .primitive = sid, .distance = t, .point = p };
}

void hswap(hit* buffer, int a, int b) {
	hit tmp = buffer[a];
	buffer[a] = buffer[b];
	buffer[b] = tmp;
}

inline void update_hit_array(const hit h, const int k, hit* hits) {

	/* It is a hit and hit is closer than the furthest hit */
	if (h.primitive >= 0 && h.distance < hits[k - 1].distance) {	
		
		/* Place hit into array and then move to place */
		hits[k - 1] = h;

		for (int j = 0; j < k - 1; j++) {

			if (h.distance < hits[j].distance)
				hswap(hits, j, k - 1);
		}
	}
}

bool heuristic(float hit) { return false; }

bool trace_k_nearest(__global const sphere* tree, const ray r, const int k, hit* hits, const int root, const bool clean) {

	/* Prepare data for algorithms - only if neceseary */
	for (int i = 0; i < k && clean; i++) {
		hits[i] = (hit) { .primitive = -1, .distance = INFINITY };
	}

	/* Tracing stack */
	MAKE_STACK(int, BVH_STACK_SIZE);
	STACK_PUSH(root);

	float alpha_accum = 0;

	/* Do BVH traversal */
	while (!STACK_IS_EMPTY()) {
	
		/* Pop top of the stack */ 
		int node = STACK_POP();

		float t = blas_trace(tree, r, node);

		/* Do not expand node if it's nearest hit is further than furthest hit */
		/* This also implicitly handles misses, as those return INFINITY automatically get discarded */
		if (t >= hits[k - 1].distance) 
			continue;


		/* Go through all the children */
		for (int child = tree[node].cbegin; child < tree[node].cend; child++) {

			if (alpha_accum >= 0.95f) {
				update_hit_array((hit) { .primitive = 0, .distance = 0 }, k, hits); 
				return true; /* Return and force exit as this ray will be opaque */
			}

			/* Prevent unneceseary expansion of leaves */
			if (tree[child].cbegin < 0 || heuristic(t)) {

				/* Child node */
				hit h = splat_trace(tree, r, child);
				update_hit_array(h, k, hits); /* Hit might get discarded here */

				if (h.primitive >= 0)
					alpha_accum += tree[child].color.w;

				continue; /* Go to next child */
			}

			/* Otherwise expand node recursively */
			if (STACK_IS_FULL()) { if (trace_k_nearest(tree, r, k, hits, child, false)) return true; } /* Propagate message upwards */
			else STACK_PUSH(child);
		}
	}
	return false;
}

__kernel void raymarch(write_only image2d_t out_img, 
					   __global const camera* cam,
					   __global const sphere* tree, const int root, 
					   const float time, const int width, const int height)
{

    int x = get_global_id(0);
    int y = get_global_id(1);

    float2 uv = (float2)(
        (float)x / (float)width, 
        (float)y / (float)height
    );

    if ((x < width) && (y < height)) {
        
		float4 px = cam->p0 + (cam->p1 - cam->p0) * uv.x
				   	    	+ (cam->p2 - cam->p0) * uv.y;
    	ray r = { cam->pos, normalize(vmul(cam->view, px)) };
		
		float alpha = 0;
		const int K = 16;
		hit hits[K];
		while(alpha < 0.95f) {

			trace_k_nearest(tree, r, K, hits, 0, true);

			if (hits[0].primitive < 0)
				break;

			int i = 0;
			for (; i < K && hits[i].primitive >= 0 && alpha < 1; i++)
				alpha += tree[hits[i].primitive].color.w;


			if (hits[K - 1].primitive < 0)
				break;
			r.origin = hits[K - 1].point; 	
		}

		if (alpha > 1) alpha = 1;
		const float4 sky = (float4)(0.52, 0.8, 0.92, 1);
		float4 blended_sky = (float4)(1, 1, 1, 1) * alpha + sky * (1 - alpha);

		write_imagef(out_img, (int2)(x, y), blended_sky);
    }
}
