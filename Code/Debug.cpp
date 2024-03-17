#include "Debug.h"

#include <stdarg.h>
#include <string.h>
#include <glm/glm.hpp>
using namespace glm;

myArray<float> &operator<<(myArray<float> &a, float b)
{
    a.push_back(b);
    return a;
}

myArray<float> &operator,(myArray<float> &a, float b)
{
    return a << b;
}

myArray<float> &operator<<(myArray<float> &a, vec4 b)
{
    return a << b.x, b.y, b.z, b.w;
}

myArray<float> &operator,(myArray<float> &a, vec4 b)
{
    return a << b;
}

myArray<float> &operator<<(myArray<float> &a, mat4 b)
{
    return a << b[0], b[1], b[2], b[3];
}

void myGui::drawRectangle(const vec4 &dst, const vec4 &src, const vec4 &col, int texSlot)
{
    _quadBuffer << dst.x, dst.y, dst.z, dst.w,
            src.x, src.y, src.z, src.w,
            uintBitsToFloat(packUnorm4x8(col)), uintBitsToFloat(texSlot);
}

bool myGui::loadFnt(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return false;
    }

    _fntInfo.resize(128);
    const char *format =
            "%*[^=]=%d%*[^=]=%d%*[^=]=%d%*[^=]=%d"
            "%*[^=]=%d%*[^=]=%d%*[^=]=%d%*[^=]=%d";
    fscanf(file, "%*[^\n]\n%*[^\n]\n%*[^\n]\n%*[^\n]\n");
    for (;;)
    {
        long cur = ftell(file);
        int a,b,c,d,e,f,g,h;
        int eof = fscanf(file, format, &a,&b,&c,&d,&e,&f,&g,&h);
        if (eof < 0) break;
        fseek(file, cur, SEEK_SET);
        fscanf(file, "%*[^\n]\n");

        _fntInfo[a] = { b,c,d,e,f,g,h };
    }

    fclose(file);
    printf("INFO: loaded file %s\n", filename);
    return true;
}

void myGui::draw2dText(const vec2 &location, float fntSize, const char *format...)
{
    const float spacing = .8;

    char textString[128];
    va_list args;
    va_start(args, format);
    vsprintf(textString, format, args);
    va_end(args);

    const int nChars = strlen(textString);
    const int spaceWid = _fntInfo['_'].xadv;
    fntSize /= spaceWid * 1.8;

    vec2 pos = vec2(0);
    for (int i=0; i<nChars; i++)
    {
        int code = textString[i];

        if (code == '\n')
        {
            pos.x = 0.;
            pos.y += fntSize * spaceWid * 1.8 * spacing;
            continue;
        }

        Glyph g = _fntInfo[code];
        vec4 src = vec4(g.xoff,g.yoff,g.w,g.h)*fntSize + vec4(pos + location, 0, 0);
        drawRectangle(src, vec4(g.x,g.y,g.w,g.h), vec4(1,1,0,0), 0);

        float xadv = code == ' ' ? spaceWid : g.xadv;
        pos.x += fntSize * xadv * spacing;
    }
}

void myDebugDraw::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &c)
{
    float col32 = uintBitsToFloat(packUnorm4x8(vec4(c.x(),c.y(),c.z(),0)));
    _lineBuffer <<
            from.x(), from.y(), from.z(), col32,
            to.x(), to.y(), to.z(), col32;
}

