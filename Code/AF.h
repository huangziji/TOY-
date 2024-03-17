#ifndef AF_H
#define AF_H
#include <glm/fwd.hpp>
using namespace glm;
#include <LinearMath/btAlignedObjectArray.h>
template <typename T> using myArray = btAlignedObjectArray<T>;

class btRigidBody;
class btTypedConstraint;
class btDynamicsWorld;

void Clear(btDynamicsWorld *);

mat4 drawPrimitive( vec3 origin, mat3 axis, vec3 halfExtend, int shapeType );

mat4 drawRigidBody( btRigidBody const& rb );

class AF
{
public:
    bool isActive_;
    myArray<btTypedConstraint *> aConstraint_;
    myArray<ivec2> aJointParentIndex_;
    myArray<vec3> aAtRestOrigin_;
    myArray<mat3> aJointAxisMod_;
};

int loadAF(AF * self, btDynamicsWorld *physics);
void deactivateAF(AF *self);

static const int AF_DATA[][11] = {
     1,-1,    0,100,0,   15,10,10,  -1,-1,-1,
     2, 0,    0,120,0,   15,10,10,  -1,-1,-1,
     3, 1,    0,140,0,   15,10,10,  -1,-1,-1,
     4, 2,    0,160,0,    3, 3, 3,  -1,-1,-1,
     0, 3,    0,163,0,   10, 2, 0,  0,185,0,

     6, 0,   10,100,0,    7,35, 0,  -1,-1,-1,
     7, 5,   10, 50,0,    7,35, 0,  -1,-1,-1,
     0, 6,   10,  0,0,    5, 5, 8,  10, 0, 8,
     9, 2,   15,155,0,    5,21, 0,  -1,-1,-1,
    10, 8,   45,155,0,    5,21, 0,  -1,-1,-1,
     0, 9,   75,155,0,    6, 2, 0,  90,155,0,

    12, 0,  -10,100,0,    7,35, 0,  -1,-1,-1,
    13,11,  -10, 50,0,    7,35, 0,  -1,-1,-1,
     0,12,  -10,  0,0,    5, 5, 8,  -10, 0, 8,
    15, 2,  -15,155,0,    5,21, 0,  -1,-1,-1,
    16,14,  -45,155,0,    5,21, 0,  -1,-1,-1,
     0,15,  -75,155,0,    6, 2, 0,  -90,155,0,
};

#endif // AF_H
