#version 320 es
precision mediump float;

#ifdef _VS
layout (location = 0) in vec4 aVertex;
void main()
{
    gl_Position = vec4(aVertex.xyz, 1);
}
#else
out vec4 fragColor;
void main()
{
    fragColor = vec4(1);
}
#endif
