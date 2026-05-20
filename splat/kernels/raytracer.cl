#include "kernels/include/debug.cl"
#include "splat/include/rtutils.h"

typedef struct __attribute__ ((packed)) _camera {
	float4 p0, p1, p2, pos;
	mat4f view;
} camera;

/* Fast Ray-Sphere algorithm from RTR */
bool rsi_fast(ray r, sphere s) {

	float4 l = s.center - r.origin;
	float _s = dot(l, r.dir),
		  lsq = dot(l, l),
		  msq = lsq - _s * _s;
	
	if ((_s < 0 && lsq > s.rsquared) || (msq > s.rsquared)) return false;
	return true;
}

/* Borrowed from https://stackoverflow.com/questions/23975555/how-to-calculate-a-ray-plane-intersection*/
hit rsi(ray r, sphere s) {

	/* Splats will always be parallel to the screen */
	float4 normal = normalize(r.origin - s.center); 

	/* Ray-Plane intersection */
	float denom = dot(r.dir, normal);	
    float t = dot(s.center - r.origin, normal) / denom;
    if (t < RT_EPSILON) return (hit) { .hit = false } ; // you might want to allow an epsilon here too

	/* Check if intersection point is within the circle */
	float4 p = r.origin + t * r.dir;
	if (dot(s.center - p, s.center - p) > s.rsquared) return (hit) { .hit = false };
	return (hit) { .hit = true, .distance = t, .point = p };
}

hit trace_tree(const ray r, __global const sphere* tree) { return (hit) {.hit = false}; } //TODO: Fill with tree traversal

void swapi(int* buffer, int a, int b) {
	int tmp = buffer[a];
	buffer[a] = buffer[b];
	buffer[b] = tmp;
}

void swaph(hit* buffer, int a, int b) {
	hit tmp = buffer[a];
	buffer[a] = buffer[b];
	buffer[b] = tmp;
}

void trace_k_nearest(__global const sphere* tree, const ray r, const int k, hit* hit_buffer, int* hits, int test) {

	/* Prepare data for algorithms */
	*hits = 0;
	for (int i = 0; i < k; i++) {
		hits[i] = -1;
		hit_buffer[i] = (hit) { .hit = false, .distance = INFINITY };
	}
	
	/* Do naive tracing as i do not have time */
	hit h;
	for (int i = 0; i < test; i++) {
		h = rsi(r, tree[i]);

		/* It is a hit and hit is closer than the furthest hit */
		if (h.hit && h.distance < hit_buffer[k - 1].distance) {
			
			/* Place hit into array and then move to place */
			hits[k - 1] = i;
			hit_buffer[k - 1] = h;


			for (int j = 0; j < k - 1; j++) {
				if (h.distance < hit_buffer[j].distance) {
					swapi(hits, j, k - 1); swaph(hit_buffer, j, k - 1);
				}
			}
		}
	}
}

__kernel void raymarch(write_only image2d_t out_img, 
					   __global const camera* cam,
					   __global const sphere* tree, const int count, 
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
		const int K = 64;
		hit hits[K];
		int hit_nodes[K];

		while (alpha < 0.95f) {
			trace_k_nearest(tree, r, K, hits, hit_nodes, count);

			if (!hits[0].hit) /* There are no more hits, end algo */
				break; 

			int i = 0;
			for (; i < K && hits[i].hit && alpha < 1; i++)
				alpha += tree[hit_nodes[i]].color.w;			

			r.origin = hits[i - 1].point; /* Continue tracing from furthest point */
		}

		const float4 sky = (float4)(0.52, 0.8, 0.92, 1);
		float4 blended_sky = (float4)(1, 1, 1, 1) * alpha + sky * (1 - alpha);

		write_imagef(out_img, (int2)(x, y), blended_sky);
    }
}
