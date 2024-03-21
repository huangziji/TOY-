#include "AF.h"
#include <btBulletDynamicsCommon.h>
#include <stdio.h>

struct IkRigNode
{
    btMatrix3x3 tipAxis;
    btVector3 tipOrigin, axisDir;
    float length;
};

myArray<IkRigNode> SamplePose(float t, const IScene *scene);

static btRigidBody *rb1,*rb2,*rb3,  *rb4,*rb5;
int Awake(btDynamicsWorld * physics)
{
    ClearWorld(physics);

    // add a plane
    physics->setGravity(btVector3(0,-10,0));
    btRigidBody *ground = new btRigidBody( 0, NULL,
        new btStaticPlaneShape(btVector3(0,1,0), -0.05) );
    ground->setDamping(.5, .5);
    ground->setFriction(.5);
    ground->setRestitution(.5);
    physics->addRigidBody(ground);

    // add an object
    rb1 = createRigidBody(btVector3(0,.60,1), btVector3(.2,.1,.02));
    rb2 = createRigidBody(btVector3(0,.35,1), btVector3(.2,.1,.02));
    rb3 = createRigidBody(btVector3(0,.1,1), btVector3(.2,.1,.02));
    rb4 = createRigidBody(btVector3(.7,.5,1), btVector3(.03,.03,.03));
    rb5 = createRigidBody(btVector3(.7,.1,1), btVector3(.02,.1,.02));

    physics->addRigidBody(rb1);
    physics->addRigidBody(rb2);
    physics->addRigidBody(rb3);
    physics->addRigidBody(rb4);
    physics->addRigidBody(rb5);

    // add a character
    AF character;
    character.Load(physics);
    character.Activate(physics);

    return 0;
}

/// @link https://allenchou.net/2018/05/game-math-swing-twist-interpolation-sterp/
void DecomposeSwingTwist(btQuaternion q, btVector3 twistAxis,
        btQuaternion & swing, btQuaternion & twist)
{
    btVector3 r(q.x(), q.y(), q.z());
    btVector3 p = twistAxis * r.dot(twistAxis) / twistAxis.length2();
    twist.setValue(p.x(), p.y(), p.z(), q.w());
    twist.normalize();
    swing = q * twist.inverse();
}

typedef struct { myArray<float> U1,W1,V2; }Output;

extern "C" void mainAnimation(Output * out, btDynamicsWorld * physics,
                              float iTime, float iTimeDelta)
{
    static int _dummy = Awake(physics);
    static const IScene *fbxAnim = loadFbx("../Data/Walking.fbx");
    static const float duration = GetAnimDuration(fbxAnim);

    myArray<IkRigNode> aIkNode =  SamplePose(fmod(iTime, duration), fbxAnim);

    float m = sin(iTime) * .0;
    btVector3 ta = btVector3(.35,0.3,1);
    btVector3 ro = ta + btVector3(sin(m), .1, cos(m)) * 1.f;

    static float t = 0;
    t += (0. < sin(iTime*3.) + .8 + sin(iTime) * .1) * .04;
    rb1->getWorldTransform().getBasis().setEulerZYX(0,iTime,0);

    btMatrix3x3 a = rb1->getWorldTransform().getBasis();
    btMatrix3x3 b = rb3->getWorldTransform().getBasis();
    btQuaternion deltaRot;
    a.timesTranspose(b).getRotation(deltaRot);
    btQuaternion swing, twist;
    DecomposeSwingTwist(deltaRot, btVector3(0,1,0), swing, twist);
    rb2->getWorldTransform().setRotation(twist.slerp(btQuaternion(0,0,0,1), .33));

    static btVector3 initialOrigin = rb4->getWorldTransform().getOrigin();
    static btVector3 initialOrigin2 = rb5->getWorldTransform().getOrigin();
    rb4->getWorldTransform().getOrigin() = initialOrigin + btVector3(cos(iTime*3)*.2, 0, 0);

    btVector3 c = rb4->getWorldTransform().getOrigin() - rb5->getWorldTransform().getOrigin();
    c.normalize();
    rb5->getWorldTransform().getBasis() = btMatrix3x3(shortestArcQuat(c, btVector3(0,1,0))).transpose();
    rb5->getWorldTransform().getOrigin() = rb5->getWorldTransform().getBasis() * btVector3(0,.1,0)
            + (initialOrigin2 - btVector3(0,.1,0));

    out->U1.clear();
    out->U1 << ta[0],ta[1],ta[2],0,ro[0],ro[1],ro[2],0;

    out->W1.clear();
    for (int i=0; i<physics->getNumCollisionObjects(); i++)
    {
        drawRigidBody( out->W1, *physics->getCollisionObjectArray()[i] );
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
