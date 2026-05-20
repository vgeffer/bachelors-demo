#include "kernels/include/math.cl"

void printv4(float4 f) {
		printf("v4 (%f %f %f %f)", f.x, f.y, f.z, f.w);
}

void printv42(float* f) {
		printf("v4 (%f %f %f %f)", f[0], f[1], f[2], f[3]);
}


void printm4(mat4f m) {
	for (int i = 0; i < 4; i++)
		{ printf("%d: ", i); printv42(m.items[i]); printf("\n"); }
}
