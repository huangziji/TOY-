#ifndef AF_H
#define AF_H
#include <LinearMath/btIDebugDraw.h>
#include <LinearMath/btAlignedObjectArray.h>
template <typename T> using myArray = btAlignedObjectArray<T>;

namespace ofbx { class IScene; } using ofbx::IScene;
class btVector3;
class btCollisionObject;
class btRigidBody;
class btDynamicsWorld;

struct IkRigNode
{
    btMatrix3x3             tipAxis;
    btVector3               tipOrigin;
    btVector3               axisDir;
    float                   length;
};

class Entity
{
    unsigned int            id_;
    btVector3               origin_;
    btMatrix3x3             axis_;
public:
};

class AF
{
    unsigned int            id_;
    bool                    isActive_;
    myArray<btVector3>      aJointOrigin_;
public:

    void Load(btDynamicsWorld *);
    void Activate(btDynamicsWorld const*);
    void UpdatePose();
};

myArray<IkRigNode> SamplePose(float t, const IScene *scene);

myArray<float> &operator<<(myArray<float> &a, float b);

myArray<float> &operator,(myArray<float> &a, float b);

void ClearWorld(btDynamicsWorld *);

btRigidBody *createBox(btVector3 origin, btVector3 halfExtents);

void drawRigidBody( myArray<float> &a, btCollisionObject const& rb );

const IScene * loadFbx(const char *filename);

float GetAnimDuration(const IScene *scene);

btVector3 GetAvgVelocityRM(const IScene *scene);

class myDebugDraw : public btIDebugDraw
{
    int debugMode_;
    myArray<float> lineBuffer_;

public:
    myArray<float> const& Data() const { return lineBuffer_; }
    void reportErrorWarning(const char* warningString) override {}
    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {}
    void draw3dText(const btVector3& location, const char* textString) override {}
    void setDebugMode(int debugMode)  override { debugMode_ = debugMode; }
    int getDebugMode() const override { return debugMode_; }
    void clearLines() override { lineBuffer_.clear(); }
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& c) override;
};

#endif // AF_H
