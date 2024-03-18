#include "AF.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
using namespace glm;
#include <stdio.h>

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
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& c) override
    {
        float col32 = uintBitsToFloat(packUnorm4x8(vec4(c.x(),c.y(),c.z(),0)));
        lineBuffer_ << from[0], from[1], from[2], col32,
                to[0], to[1], to[2], col32;
    }
};

int Awake(btDynamicsWorld * physics)
{
    Clear(physics);

    // add a plane
    physics->setGravity(btVector3(0,-10,0));
    btRigidBody *ground = new btRigidBody( 0, NULL,
        new btStaticPlaneShape(btVector3(0,1,0), -0.05) );
    ground->setDamping(.5, .5);
    ground->setFriction(.5);
    ground->setRestitution(.5);
    physics->addRigidBody(ground);

    // add an object
    btVector3 origin = btVector3(0,5,0), halfExtents = btVector3(.1,.1,.1);
    float mass = 1.0;
    btVector3 inertia;
    btTransform x, _dummy;
    x.setIdentity();
    _dummy.setIdentity();
    x.setOrigin(origin);
    btCollisionShape *shape = new btBoxShape(halfExtents);
    shape->calculateLocalInertia(mass, inertia);
    btRigidBody *rb = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
        mass, new btDefaultMotionState(x, _dummy), shape, inertia));
    physics->addRigidBody(rb);

    // add a character
    AF af;
    loadAF(&af, physics);
    deactivateAF(&af, physics);

    return 0;
}

typedef struct { myArray<float> U1, W1,V2; }Output;

extern "C" void mainAnimation(Output * out, btDynamicsWorld * physics,
                              float iTime, float iTimeDelta)
{
    static int _dummy = Awake(physics);
// static const IScene *fbxAnim = loadFbx("../Data/Walking.fbx");

    btVector3 ta = btVector3(0,1.,0);
    btVector3 ro = ta + btVector3(sin(iTime), .3, cos(iTime)) * 2.f;

    out->U1.clear();
    out->U1 << ta[0],ta[1],ta[2],0,ro[0],ro[1],ro[2],0;

    out->W1.clear();
    for (int i=0; i<physics->getNumCollisionObjects(); i++)
    {
        out->W1 << drawRigidBody( *physics->getCollisionObjectArray()[i] );
    }

    static myDebugDraw *dd = new myDebugDraw;
    const int flag = btIDebugDraw::DBG_DrawConstraints;//|btIDebugDraw::DBG_DrawWireframe;
    dd->clearLines();
    dd->setDebugMode(flag);
    physics->setDebugDrawer(dd);
    physics->debugDrawWorld();
    physics->stepSimulation(iTimeDelta);
    out->V2.copyFromArray(dd->Data());
}
