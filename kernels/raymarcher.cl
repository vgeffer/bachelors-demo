#include "kernels/include/math.cl"
#include "kernels/include/rt_primitives.cl"

typedef struct __attribute__ ((packed)) _camera {
	float4 p0, p1, p2, pos;
	mat4f view;
} camera;

float sphereSDF(float3 p, float3 c, float sqradius) {
	return length(p - c) - sqradius;
}

float boxSDF(float3 p, float3 c, float3 s) {
  	float3 v = fabs(p - c) - s;
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

float sceneSDF(float3 p, float time) {
   	return differenceSDF(
		boxSDF(p, (float3)(0, 0, 0), (float3)(0.7, 0.7, 0.7)),	
		intersectSDF(
			boxSDF(p, (float3)(0, 0, 0), (float3)(1, 1, 1)),
   			sphereSDF(p + (float3)(0, sin(time), 0), (float3)(0, 0, 0), 1.4)
		)
	);
}

float4 pixel(float2 uv, __global const camera* cam, float time) {
	float4 px = cam->p0 + (cam->p1 - cam->p0) * uv.x
				   	    + (cam->p2 - cam->p0) * uv.y;
    ray r = { cam->pos, normalize(vmul(cam->view, px)) };
	float depth = 0;
  	
    for (float step = 0; step < 4096; step++) {

	  	float3 point = r.origin.xyz + depth * r.dir.xyz;
	  	float dist = sceneSDF(point, time);
	  	
        if (dist < 0.001) 
		  	return (float4)(0, 1 - (step / 4096), 0, 1);
	  	depth += dist;
	  	
        if (depth > 100)
		  	break;
  	}

  	return (float4)(0, 0, 0, 1);
}

__kernel void raymarch(write_only image2d_t out_img, 
					   __global const camera* cam,
					   const float time, const int width, const int height)
{

    int x = get_global_id(0);
    int y = get_global_id(1);

    float2 uv = (float2)(
        (float)x / (float)width, 
        (float)y / (float)height
    );

    if ((x < width) && (y < height)) {
        
        write_imagef(out_img, (int2)(x, y), pixel(uv, cam, time));
    }
}