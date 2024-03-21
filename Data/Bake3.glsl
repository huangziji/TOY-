#version 320 es

layout (location = 0) uniform uvec3 iResolution;
layout (binding = 0) uniform atomic_uint uCount;
layout (std430, binding = 0) buffer IN_0 { vec4 data[]; };
layout (local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main(void)
{
    uvec3 id = gl_GlobalInvocationID;
    data[atomicCounterIncrement(uCount)] = vec4(0);
}
