#include "AF.h"
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>
#include <stdio.h>

static btRigidBody *rb1,*rb2,*rb3,  *rb4,*rb5;
int Awake(btSoftRigidDynamicsWorld * physics)
{
    ClearWorld(physics);

    physics->setGravity(btVector3(0,-10,0));
    btRigidBody *ground = new btRigidBody( 0, NULL,
        new btStaticPlaneShape(btVector3(0,1,0), -0.05) );
    ground->setDamping(.5, .5);
    ground->setFriction(.5);
    ground->setRestitution(.5);
    physics->addRigidBody(ground);

    // btSoftBodyWorldInfo & info = physics->getWorldInfo();
    // btSoftBody *cloth = new btSoftBody(&info);
    // cloth->getCollisionShape()->setMargin(.1);
    // physics->addSoftBody(cloth);

    // add an object
    rb1 = createBox(btVector3(0,.60,1), btVector3(.2,.1,.02));
    rb2 = createBox(btVector3(0,.35,1), btVector3(.2,.1,.02));
    rb3 = createBox(btVector3(0,.1,1), btVector3(.2,.1,.02));
    rb4 = createBox(btVector3(.7,.5,1), btVector3(.03,.03,.03));
    rb5 = createBox(btVector3(.7,.1,1), btVector3(.02,.1,.02));

    physics->addRigidBody(rb1);
    physics->addRigidBody(rb2);
    physics->addRigidBody(rb3);
    physics->addRigidBody(rb4);
    physics->addRigidBody(rb5);

    // add a character
    AF character;
    character.Load(physics);
    // character.Activate(physics);

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

btMatrix3x3 setCamera(btVector3 ro, btVector3 ta, float cr)
{
    using vec3 = btVector3;
    vec3 cw = (ta-ro).normalize();
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = (btCross(cw, cp)).normalize();
    vec3 cv = btCross(cu, cw);
    return btMatrix3x3(cu, cv, cw);
}

extern "C" void mainAnimation(Output * out, btSoftRigidDynamicsWorld * physics,
        float iTime, float iTimeDelta, float iResolutionX, float iResolutionY, float iMouseX, float iMouseY)
{
    static int _dummy = Awake(physics);
    static const IScene *fbxAnim = loadFbx("../Data/Walking.fbx");
    static const float duration = GetAnimDuration(fbxAnim);

    myArray<IkRigNode> aIkNode =  SamplePose(fmod(iTime, duration), fbxAnim);

    float m = sin(iTime) * .0;
    btVector3 ta = btVector3(.35,1.55,0);
    btVector3 ro = ta + btVector3(sin(m), .4, cos(m)) * 2.3f;

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
    const int flag = btIDebugDraw::DBG_DrawConstraints|btIDebugDraw::DBG_DrawWireframe;
    dd->clearLines();
    dd->setDebugMode(flag);
    physics->setDebugDrawer(dd);
    physics->debugDrawWorld();
    physics->stepSimulation(iTimeDelta);


    btMatrix3x3 ca = setCamera(ro, ta, 0.0);
    btVector3 rd = ca * btVector3(
                 (2.0 * iMouseX - iResolutionX) / iResolutionY,
                -(2.0 * iMouseY - iResolutionY) / iResolutionY,
                1.2).normalize();
    btVector3 rayFrom = ro + rd * 0.1;
    btVector3 rayTo = ro + rd * 1000.;
    btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);
    physics->rayTest(rayFrom, rayTo, rayCallback);
    if (rayCallback.hasHit())
    {
        dd->drawSphere(rayCallback.m_collisionObject->getWorldTransform().getOrigin(), .1, {1,1,0});
        btRigidBody *rb = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject);

        btTransform x;
        x.setIdentity();
        x.setOrigin(rayCallback.m_hitPointWorld);
        static btGeneric6DofConstraint *cons = new btGeneric6DofConstraint(*rb, x, false);
        cons->setLinearLowerLimit(btVector3(0,0,0));
        cons->setLinearUpperLimit(btVector3(0,0,0));
        cons->setAngularUpperLimit(btVector3(0,0,0));
        cons->setAngularUpperLimit(btVector3(0,0,0));
        cons->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 0);
        cons->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 1);
        cons->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 2);
        cons->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 3);
        cons->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 4);
        cons->setParam(BT_CONSTRAINT_STOP_CFM, 0.8f, 5);
        cons->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 0);
        cons->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 1);
        cons->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 2);
        cons->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 3);
        cons->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 4);
        cons->setParam(BT_CONSTRAINT_STOP_ERP, 0.1f, 5);

        cons->getRigidBodyA() = *rb;

        // btVector3 oldOrigin = cons->getFrameOffsetA().getOrigin();
        float t = (rayCallback.m_hitPointWorld - ro).length();
        btVector3 newOrigin = ro + rd*t;
        cons->getFrameOffsetA().setOrigin(newOrigin);
    }


    out->V2.copyFromArray(dd->Data());
}
