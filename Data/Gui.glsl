#version 320 es
precision mediump float;

#ifdef _VS
#define _varying out
#else
#define _varying in
#endif

_varying vec2 UV;
flat _varying vec4 vCol;
flat _varying uint vId;
layout (location = 0) uniform vec2 iResolution;
layout (binding  = 4) uniform sampler2D iChannel4;
layout (binding  = 5) uniform sampler2D iChannel5;

#ifdef _VS
layout (location = 1) in vec4 aDst;
layout (location = 2) in vec4 aSrc;
layout (location = 3) in uvec2 aColxId;
void main()
{
    vCol = unpackUnorm4x8(aColxId.x);
    vId = aColxId.y;

    UV = vec2(gl_VertexID&1, gl_VertexID/2).yx; // vflip change the winding

    vec2 pos = aDst.xy + aDst.zw * UV;
    UV = aSrc.xy + aSrc.zw * UV;
    pos /= iResolution;

    gl_Position = vec4(pos * 2. - 1., 0, 1);
    gl_Position.y *= -1.; // vflip
}
#else
out vec4 fragColor;
void main()
{
    if (vId == 0U)
    {
        vec2 texSize = vec2(textureSize(iChannel4, 0));
        float d = textureLod(iChannel4, UV/texSize, 0.).r;
        fragColor = vec4(vCol.xyz, smoothstep(.4, .5, d));
    }
    else
    {
        vec2 texSize = vec2(textureSize(iChannel5, 0));
        float d = textureLod(iChannel5, UV/texSize, 0.).r;
        fragColor = vec4(vCol.xyz, d);
    }
}
#endif
