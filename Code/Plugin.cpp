#include "AF.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
using namespace glm;
#include <stdio.h>

myArray<float> &operator<<(myArray<float> &a, float b);
myArray<float> &operator,(myArray<float> &a, float b);
myArray<float> &operator<<(myArray<float> &a, vec4 b);
myArray<float> &operator, (myArray<float> &a, vec4 b);
myArray<float> &operator<<(myArray<float> &a, mat4 b);

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
    return 0;
}

namespace ofbx { class IScene; }
using namespace ofbx;

const IScene * loadFbx(const char *filename);
float GetAnimDuration(const IScene *scene);
btVector3 GetAvgVelocityRM(const IScene *scene);

typedef struct { myArray<float> U1, W1; }Output;

extern "C" void mainAnimation(Output * out, btDynamicsWorld * physics,
                              float iTime, float iTimeDelta)
{
    static int _dummy = Awake(physics);
    static const IScene *fbxAnim = loadFbx("../Data/Walking.fbx");

    btVector3 avgVel = GetAvgVelocityRM(fbxAnim);
    printf("avgVel : %f %f %f\n", avgVel.x(), avgVel.y(), avgVel.z());
    printf("durati : %f\n", GetAnimDuration(fbxAnim));

    vec3 ta = vec3(0,1.,0);
    vec3 ro = ta + vec3(sin(iTime), .3, cos(iTime)) * 2.f;
    out->U1.clear();
    out->U1 << ta.x,ta.y,ta.z,0,ro.x,ro.y,ro.z,0;

    static AF af;
    static long lastMod;
    static int _dummy2 = loadAF(&af, physics);
    // deactivateAF(&af);

    btGeneric6DofConstraint *joint = dynamic_cast<btGeneric6DofConstraint *>(af.aConstraint_[0]);
    joint->setLinearLowerLimit(btVector3(0,0,0));
    joint->setLinearLowerLimit(btVector3(0,0,0));
    joint->setAngularLowerLimit(btVector3(0,0,0));
    joint->setAngularUpperLimit(btVector3(0,0,0));
    for (int i=0; i<6; i++)
    {
        // joint->setParam(BT_CONSTRAINT_STOP_CFM, .8, i);
        joint->setParam(BT_CONSTRAINT_STOP_ERP, .1, i);
    }

    btIDebugDraw *dd = physics->getDebugDrawer();
    // dd->clearLines();
    const int flag = btIDebugDraw::DBG_DrawConstraints;//|btIDebugDraw::DBG_DrawWireframe;
    dd->setDebugMode(flag);
    physics->debugDrawWorld();
    physics->stepSimulation(iTimeDelta);


    out->W1.clear();
    out->W1 << drawPrimitive(vec3(0,0,0), mat3(1), vec3(0.05), 3);

    btVector3 pos = (joint->getRigidBodyA().getWorldTransform() * joint->getFrameOffsetA()).getOrigin();
    // vec3 local = -(af.aAtRestOrigin_[1]-af.aAtRestOrigin_[0]);
    // btVector3 qos = pos + joint->getRigidBodyB().getWorldTransform().getBasis() * btVector3(local.x, local.y, local.z);
    // dd->drawSphere(qos, .04, {1,0,1});
    out->W1 << drawRigidBody( joint->getRigidBodyB() );

    for (int i=0; i<af.aConstraint_.size(); i++)
    {
        const btGeneric6DofConstraint *joint = dynamic_cast<const btGeneric6DofConstraint *>(af.aConstraint_[i]);
        btTransform xform = joint->getRigidBodyA().getWorldTransform() * joint->getFrameOffsetA();
        out->W1 << drawRigidBody( joint->getRigidBodyA() );
        // dd->drawSphere(xform.getOrigin(), .03, {1,1,0});
    }
}
