#ifndef DEBUG_H
#define DEBUG_H
#include <LinearMath/btIDebugDraw.h>
#include <LinearMath/btAlignedObjectArray.h>
#include <glm/fwd.hpp>

using glm::vec2;
using glm::vec4;
template <typename T> using myArray = btAlignedObjectArray<T>;

class myGui
{
    typedef struct{ int x,y,w,h,xoff,yoff,xadv; }Glyph;

    myArray<Glyph> _fntInfo;
    myArray<float> _quadBuffer;
public:
    void clearQuads() { _quadBuffer.clear(); }
    myArray<float> const& getQuadBuffer() const { return _quadBuffer; }
    bool loadFnt(const char *filename);
    void draw2dText(const vec2 &location, float fntSize, const char *fmt...);
    void drawRectangle(const vec4 &dst, const vec4 &src, const vec4 &col, int texSlot = 1);
};

class myDebugDraw : btIDebugDraw
{
    int _debugMode;
    myArray<float> _lineBuffer;

    void reportErrorWarning(const char* warningString) override final {}
    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override final {}
    void draw3dText(const btVector3& location, const char* textString)  override final {}
public:
    void setDebugMode(int debugMode)  override final { _debugMode = debugMode; }
    int getDebugMode() const override final { return _debugMode; }
    void clearLines() override final { _lineBuffer.clear(); }
    myArray<float> const& getLineBuffer() const { return _lineBuffer; }

    void drawLine(const btVector3& from, const btVector3& to, const btVector3& c) override final;
};

#endif // DEBUG_H
