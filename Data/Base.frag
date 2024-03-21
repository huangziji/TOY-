#version 320 es
precision mediump float;

// https://iquilezles.org/articles/palettes/
vec3 palette(float t)
{
    float TWOPI = 3.14159 * 2.;
    return 0.5 - 0.5 * cos(TWOPI * (vec3(0.8,0.9,0.9) * t + vec3(0.2,0.5,0.0)));
}

float saturate(float a)
{
    return clamp(a,0.,1.);
}

mat3 setCamera(in vec3 ro, in vec3 ta, float cr)
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = cross(cu, cw);
    return mat3(cu, cv, cw);
}

struct Hit
{
    float t;
    vec3 nor;
    int m;
};

Hit castRay(in vec3 ro, in vec3 rd);

out vec4 fragColor;
layout (std140, binding = 1) uniform VIEW { vec4 uTarget, uOrigin; };
layout (location = 0) uniform vec2 iResolution;
void main()
{
    vec2 uv = (2.0*gl_FragCoord.xy-iResolution.xy) / iResolution.y;

    vec3 ta = uTarget.xyz;
    vec3 ro = uOrigin.xyz;
    mat3 ca = setCamera(ro, ta, 0.0);
    vec3 rd = ca * normalize(vec3(uv, 1.2));

    Hit h = castRay(ro, rd);
    float t = h.t;

    vec3 col = vec3(0);
    if (t < 100.)
    {
        const vec3 sun_dir = normalize(vec3(3,2,2));

        vec3 nor = h.nor;
        vec3 pos = ro + rd*t;
        float sha = step( 100., castRay(pos + nor*.003, sun_dir).t )*.5+.5;
        vec3 mate = palette(float(h.m) + 113124.142311);

        float sun_dif = saturate(dot(nor, sun_dir))*.8+.2;
        float sky_dif = saturate(dot(nor, vec3(0,1,0)))*.15;
        col += vec3(0.9,0.9,0.5)*sun_dif*sha*mate;
        col += vec3(0.5,0.6,0.9)*sky_dif;
    }
    else
    {
        col += vec3(0.5,0.6,0.9) - rd.y*.4;
    }

    col = pow(col, vec3(.4545));
    fragColor = vec4(col, 1);

    const float n = 0.1, f = 1000.0;
    const float p10 = (f+n)/(f-n), p11 = -2.0*f*n/(f-n); // from perspective matrix
    float ssd = min(t, f) * dot(rd, normalize(ta-ro)); // convert camera dist to screen space dist
    float ndc = p10 + p11/ssd; // inverse of linear depth
    gl_FragDepth = (ndc*gl_DepthRange.diff + gl_DepthRange.near + gl_DepthRange.far) * 0.5;
}

layout (location = 4) uniform int uCount;
layout (std430, binding  = 1) buffer IN_0 { mat4 aInstance[]; };

vec4 plaIntersect( in vec3 ro, in vec3 rd, in vec4 p );
vec4 boxIntersect( in vec3 ro, in vec3 rd, in vec3 h );
vec4 sphIntersect( in vec3 ro, in vec3 rd, float ra );
vec4 capIntersect( in vec3 ro, in vec3 rd, float ra, float he );

Hit castRay(in vec3 ro, in vec3 rd)
{
    float t = 1e30f;
    vec3 nor;
    int id;

    for (int i=0; i<uCount; i++)
    {
        mat4 x = aInstance[i];
        mat3 axis = mat3(x);
        vec3 origin = x[3].xyz;
        float a = x[0][3], b = x[1][3], c = x[2][3];
        int shapeType = floatBitsToInt(x[3][3]);

        vec4 h;
        vec3 O = axis * (ro - origin);
        vec3 D = axis * rd;
        switch (shapeType)
        {
        case 0:
            h = plaIntersect(O, D, vec4(0,1,0,-a));
            break;
        case 1:
            h = boxIntersect(O, D, vec3(a,b,c));
            break;
        case 2:
            h = sphIntersect(O, D, a);
            break;
        case 3:
            h = capIntersect(O, D, a, b);
            break;
        default:
            break;
        }

        if (0. < h.x && h.x < t)
        {
            t = h.x;
            nor = h.yzw * axis;
            id = i;
        }
    }
    return Hit(t, nor, id);
}

vec4 plaIntersect( in vec3 ro, in vec3 rd, in vec4 p )
{
    float t = -(dot(ro,p.xyz)+p.w)/dot(rd,p.xyz);
    return vec4(t, p.xyz);
}
vec4 boxIntersect( in vec3 ro, in vec3 rd, vec3 h )
{
    vec3 m = 1.0/rd;
    vec3 n = m*ro;
    vec3 k = abs(m)*h;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
    float tN = max( max( t1.x, t1.y ), t1.z );
    float tF = min( min( t2.x, t2.y ), t2.z );
    if( tN>tF || tF<0.0) return vec4(1e30f);
    vec3 nor = (tN>0.0) ? step(vec3(tN),t1) : step(t2,vec3(tF));
        nor *= -sign(rd);
    return vec4( tN, nor );
}
vec4 sphIntersect( in vec3 ro, in vec3 rd, float ra )
{
    float b = dot( ro, rd );
    vec3 qc = ro - b*rd;
    float h = ra*ra - dot( qc, qc );
    if( h<0.0 ) return vec4(-1.0);
    h = sqrt( h );
    float t = -b-h;
    return vec4(t, (ro + t*rd)/ra);
}
vec4 capIntersect( in vec3 ro, in vec3 rd, float ra, float he )
{
    float k2 = 1.0        - rd.y*rd.y;
    float k1 = dot(ro,rd) - ro.y*rd.y;
    float k0 = dot(ro,ro) - ro.y*ro.y - ra*ra;

    float h = k1*k1 - k2*k0;
    if( h<0.0 ) return vec4(-1.0);
    h = sqrt(h);
    float t = (-k1-h)/k2;

    // body
    float y = ro.y + t*rd.y;
    if( y>-he && y<he ) return vec4( t, (ro + t*rd - vec3(0.0,y,0.0))/ra );

    // caps
    vec3 oc = ro + vec3(0.0,he,0.0) * -sign(y);
    float b = dot(rd,oc);
    float c = dot(oc,oc) - ra*ra;
    h = b*b - c;
    t = -b - sqrt(h);
    if( h>0.0 ) return vec4( t, (oc + t*rd)/ra );

    return vec4(-1.0);
}
