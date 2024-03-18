#ifndef AF_H
#define AF_H
#include <glm/fwd.hpp>
using namespace glm;
#include <LinearMath/btAlignedObjectArray.h>
template <typename T> using myArray = btAlignedObjectArray<T>;

class btVector3;
class btCollisionObject;
class btDynamicsWorld;

class AF
{
public:
    uint id_;
    bool isActive_;
    myArray<vec3> aAtRestOrigin_;
    myArray<ivec2> aJointParentIndex_;
};

myArray<float> &operator<<(myArray<float> &a, float b);
myArray<float> &operator,(myArray<float> &a, float b);
myArray<float> &operator<<(myArray<float> &a, vec4 b);
myArray<float> &operator, (myArray<float> &a, vec4 b);
myArray<float> &operator<<(myArray<float> &a, mat4 b);

int loadAF(AF * self, btDynamicsWorld *physics);

void deactivateAF(AF *self, const btDynamicsWorld * physics);

void Clear(btDynamicsWorld *);

mat4 drawPrimitive( vec3 origin, mat3 axis, vec3 halfExtend, int shapeType );

mat4 drawRigidBody( btCollisionObject const& rb );

namespace ofbx { class IScene; } using ofbx::IScene;

const IScene * loadFbx(const char *filename);

float GetAnimDuration(const IScene *scene);

btVector3 GetAvgVelocityRM(const IScene *scene);

#endif // AF_H
