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
layout (binding  = 4) uniform sampler2D iChannel[2];

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
    vec2 texSize = vec2(textureSize(iChannel[vId], 0));
    float d = textureLod(iChannel[vId], UV/texSize, 0.).r;
    float t = smoothstep(.4, .5, d);
    fragColor = vec4(vCol.xyz, max(vCol.w, t));
}
#endif
