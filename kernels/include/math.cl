

typedef struct __attribute__((packed)) _mat4f {
    union {
        float items[4][4]; /* column, row */
        float4 columns[4]; 
    };
} mat4f;

mat4f identity() {
    
    mat4f out;
    out.columns[0] = (float4)(1, 0, 0, 0);
    out.columns[1] = (float4)(0, 1, 0, 0);
    out.columns[2] = (float4)(0, 0, 1, 0);
    out.columns[3] = (float4)(0, 0, 0, 1);
    
    return out;
};

mat4f transpose(mat4f a) {

    mat4f out;
    for (int i = 0; i < 4; i++ )
        out.columns[i] = (float4)(a.items[0][i], a.items[1][0], a.items[2][i], a.items[3][i]);

    return out;
}

float4 vmul(mat4f a, float4 b) {
        
    return (float4)(
        a.columns[0] * b.x + 
        a.columns[1] * b.y +
        a.columns[2] * b.z +
        a.columns[3] * b.w
    ); 
}
    
mat4f mmul(mat4f a, mat4f b) {
    
    mat4f out;
    for (int i = 0; i < 4; i++ )
        out.columns[i] = vmul(a, a.columns[i]);

    return out; 
}