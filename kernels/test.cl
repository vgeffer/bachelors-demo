__kernel void test(write_only image2d_t outimg, const float time, const int buffer_width, const int buffer_height)
{

    int x = get_global_id(0);
    int y = get_global_id(1);

    float2 uv = (float2)(
        (float)x / (float)buffer_width, 
        (float)y / (float)buffer_height
    );

    if ((x < buffer_width) && (y < buffer_height)) {
        
        write_imagef(outimg, (int2)(x, y), (float4)(uv.x * fabs(sin(time * 0.4273890)), uv.y * fabs(sin(time * 0.64234987)), fabs(sin(time * 0.123921)), 1));
    }
}