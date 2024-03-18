#include "Debug.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <glm/glm.hpp>
using namespace glm;

inline myArray<int> &operator<<(myArray<int> &a, int b)
{
    a.push_back(b);
    return a;
}

inline myArray<int> &operator,(myArray<int> &a, int b)
{
    return a << b;
}

void myGui::drawRectangle(ivec4 const& dst, ivec4 const& src, int col, int texSlot)
{
    _quadBuffer <<
            (dst.x | (dst.y << 16)), (dst.z | (dst.w << 16)),
            (src.x | (src.y << 16)), (src.z | (src.w << 16)),
            col, texSlot, 0,0;
}

bool myGui::loadFnt(const char *filename)
{
    const time_t stop = clock();
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
    const time_t elapseTime = (clock()-stop) / 1000;
    printf("INFO: loaded file %s. It took %d ms\n", filename, elapseTime);
    return true;
}

void myGui::draw2dText(int posX, int posY, int col, int fntSize, const char *format...)
{
    char textString[128];
    va_list args;
    va_start(args, format);
    vsprintf(textString, format, args);
    va_end(args);

    const float spacing = 0;
    const int nChars = strlen(textString);
    const int charWid = _fntInfo['_'].xadv;
    const int lineHei = charWid * 2;

    for (int x=0, y=0, i=0; i<nChars; i++)
    {
        int code = textString[i];
        if (code == '\n')
        {
            x = 0;
            y += fntSize;
            continue;
        }

        Glyph g = _fntInfo[code];
        ivec4 dst = ivec4(g.xoff,g.yoff,g.w,g.h) * fntSize / lineHei
                + ivec4(x + posX, y + posY, 0, 0);
        drawRectangle(dst, ivec4(g.x,g.y,g.w,g.h), col, 0);

        int xadv = code == ' ' ? charWid : g.xadv;
        x += xadv * fntSize / lineHei + spacing;
    }
}
