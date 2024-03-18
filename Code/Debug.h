#ifndef DEBUG_H
#define DEBUG_H
#include <glm/fwd.hpp>
using namespace glm;
#include <LinearMath/btAlignedObjectArray.h>
template <typename T> using myArray = btAlignedObjectArray<T>;

class myGui
{
    typedef struct{ int x,y,w,h,xoff,yoff,xadv; }Glyph;
    myArray<Glyph> _fntInfo;
    myArray<int> _quadBuffer;
public:
    myArray<int> const& Data() const { return _quadBuffer; }
    void clearQuads() { _quadBuffer.clear(); }
    bool loadFnt(const char *filename);
    void draw2dText(int posX, int posY, int col, int fntSize, const char *fmt...);
    void drawRectangle(ivec4 const& dst, ivec4 const& src, int col, int texSlot = 1);
};

#endif // DEBUG_H
