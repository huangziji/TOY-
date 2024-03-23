#include "AF.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
using namespace glm;

btRigidBody *createBody(btVector3 origin, btCollisionShape *shape)
{
    float mass = 1.0;
    btVector3 inertia;
    shape->calculateLocalInertia(mass, inertia);
    btRigidBody *rb = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
        mass, NULL, shape, inertia));
    btTransform x;
    x.setOrigin(origin);
    x.getBasis().setEulerZYX(0,0,0);
    rb->setWorldTransform(x);
    rb->setActivationState(btRigidBody::CF_KINEMATIC_OBJECT);
    return rb;
}

btRigidBody *createBox(btVector3 origin, btVector3 halfExtents)
{
    return createBody(origin, new btBoxShape(halfExtents));
}

btRigidBody *createCapsule(btVector3 origin, float radius, float height)
{
    return createBody(origin, new btCapsuleShape(radius, height));
}


btTypedConstraint *createJoint1(btVector3 pivot)
{
    btRigidBody *rb1, *rb2;
    btTransform localA, localB;
    localA.setOrigin(rb1->getWorldTransform().invXform(pivot));
    localB.setOrigin(rb2->getWorldTransform().invXform(pivot));
    localA.setBasis(rb1->getWorldTransform().getBasis().transpose());
    localB.setBasis(rb2->getWorldTransform().getBasis().transpose());
    btHingeConstraint *joint = new btHingeConstraint(*rb1, *rb2, localA, localB);
    joint->setLimit(0,0);
    return joint;
}

btTypedConstraint *createJoint2(btVector3 pivot)
{
    btRigidBody *rb1, *rb2;
    btTransform localA, localB;
    localA.setOrigin(rb1->getWorldTransform().invXform(pivot));
    localB.setOrigin(rb2->getWorldTransform().invXform(pivot));
    localA.setBasis(rb1->getWorldTransform().getBasis().transpose());
    localB.setBasis(rb2->getWorldTransform().getBasis().transpose());
    btConeTwistConstraint *joint = new btConeTwistConstraint(*rb1, *rb2, localA, localB);
    joint->setLimit(0,0,0);
    return joint;
}

void myDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& c)
{
    float code =  uintBitsToFloat(packUnorm4x8(vec4(c.x(),c.y(),c.z(),0)));
    lineBuffer_ << from[0], from[1], from[2], code,
            to[0], to[1], to[2], code;
}

myArray<float> &operator<<(myArray<float> &a, float b)
{
    a.push_back(b);
    return a;
}

myArray<float> &operator,(myArray<float> &a, float b)
{
    return a << b;
}

void ClearWorld(btDynamicsWorld *physics)
{
    for (int i = physics->getNumConstraints() - 1; i >= 0; i--)
    {
        btTypedConstraint *joint = physics->getConstraint(i);
        physics->removeConstraint(joint);
        delete joint;
    }
    for (int i = physics->getNumCollisionObjects() - 1; i >= 0; i--)
    {
        btCollisionObject *obj = physics->getCollisionObjectArray()[i];
        btRigidBody *body = btRigidBody::upcast(obj);
        if (body)
        {
            if (body->getMotionState())
                delete body->getMotionState();
            if (body->getCollisionShape())
                delete body->getCollisionShape();
        }
        physics->removeCollisionObject(obj);
        delete obj;
    }
}

void drawRigidBody( myArray<float> & a, btCollisionObject const& rb )
{
    int shapeType;
    btVector3 halfExtents;
    switch (rb.getCollisionShape()->getShapeType())
    {
    case STATIC_PLANE_PROXYTYPE:
        shapeType = 0,
        halfExtents[0] = ((btStaticPlaneShape*)rb.getCollisionShape())->getPlaneConstant();
        break;
    case BOX_SHAPE_PROXYTYPE:
        shapeType = 1,
        halfExtents = ((btBoxShape*)rb.getCollisionShape())->getHalfExtentsWithMargin();
        break;
    case SPHERE_SHAPE_PROXYTYPE:
        shapeType = 2,
        halfExtents[0] = ((btCapsuleShape*)rb.getCollisionShape())->getRadius();
        break;
    case CAPSULE_SHAPE_PROXYTYPE:
        shapeType = 3,
        halfExtents[0] = ((btCapsuleShape*)rb.getCollisionShape())->getRadius(),
        halfExtents[1] = ((btCapsuleShape*)rb.getCollisionShape())->getHalfHeight();
        break;
    }

    btVector3 origin = rb.getWorldTransform().getOrigin();
    btMatrix3x3 axis = rb.getWorldTransform().getBasis();
    a << axis[0][0], axis[0][1], axis[0][2], halfExtents[0],
        axis[1][0], axis[1][1], axis[1][2], halfExtents[1],
        axis[2][0], axis[2][1], axis[2][2], halfExtents[2],
        origin[0], origin[1], origin[2], intBitsToFloat(shapeType);
}

void AF::Load(btDynamicsWorld *physics)
{
    const int X[] = {
    /*  idx        origin   limits       bounds    rz   offset */
        -1,       0,100,0,              16,10,12,  0,  0,10, 0,
         0,       0,120,0,              16,10,12,  0,  0,10, 0,
         1,       0,140,0,              16,10,12,  0,  0,10, 0,
         2,       0,160,0,               3, 2, 3,  0,  0, 2, 0,
         3,       0,164,0,              10, 2, 0,  0,  0,12, 0,

         0,      10,100,0,               7,35, 0,  0,  0,-25,0,
         5,      10, 50,0,               7,35, 0,  0,  0,-25,0,
         6,      10,  0,0,               5, 5, 8,  0,  0, 0, 4,
         2,      15,155,0,               5,21, 0, 90,  15,0, 0,
         8,      45,155,0,               5,21, 0, 90,  15,0, 0,
         9,      75,155,0,               6, 2, 0, 90,  6, 0, 0,

         0,     -10,100,0,               7,35, 0,  0,  0,-25,0,
        11,     -10, 50,0,               7,35, 0,  0,  0,-25,0,
        12,     -10,  0,0,               5, 5, 8,  0,  0, 0, 4,
         2,     -15,155,0,               5,21, 0, 90,  -15,0,0,
        14,     -45,155,0,               5,21, 0, 90,  -15,0,0,
        15,     -75,155,0,               6, 2, 0, 90,  -6, 0,0,
    };

    isActive_ = false;
    aJointOrigin_.clear();
    id_ = physics->getNumCollisionObjects();

    for (int i=0; i<countof(X); i+=11)
    {
        int a = X[i+0], b = X[i+1], c = X[i+2],
            d = X[i+3], e = X[i+4], f = X[i+5],
            g = X[i+6], h = X[i+7], j = X[i+8],
            k = X[i+9], l = X[i+10];

        float rz = btRadians(h);
        btVector3 origin = btVector3(b,c,d) * 0.01f;
        btVector3 bounds = btVector3(e,f,g) * 0.01f;
        btVector3 offset = btVector3(j,k,l) * 0.01f;

        btCollisionShape *shape = bounds[2] > 0.0f ?
            (btCollisionShape *)new btBoxShape(bounds) :
            (btCollisionShape *)new btCapsuleShape(bounds[0], bounds[1]);
        float mass = 1.0;
        btVector3 inertia;
        shape->calculateLocalInertia(mass, inertia);
        btRigidBody *rb1 = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
            mass, NULL, shape, inertia));
        btTransform x;
        x.setIdentity();
        x.setOrigin(origin + offset);
        x.getBasis().setEulerZYX(0, 0, rz);
        rb1->setWorldTransform(x);
        rb1->setActivationState(btRigidBody::CF_KINEMATIC_OBJECT);
        physics->addRigidBody(rb1);

        if (i)
        {
            btRigidBody *rb2 = btRigidBody::upcast(physics->getCollisionObjectArray()[id_ + a]);
            btTransform localA;
            btTransform localB;
            localA.setBasis( rb1->getWorldTransform().getBasis().transpose() );
            localB.setBasis( rb2->getWorldTransform().getBasis().transpose() );
            localA.setOrigin( rb1->getWorldTransform().invXform(origin) );
            localB.setOrigin( rb2->getWorldTransform().invXform(origin) );
            btHingeConstraint *joint = new btHingeConstraint(*rb1, *rb2, localA, localB);
            btMatrix3x3 axis;
            axis.setIdentity();
            axis.setEulerZYX(0,M_PI_2,0);
            joint->getFrameOffsetA().getBasis() *= axis;
            joint->getFrameOffsetB().getBasis() *= axis;
            joint->setLimit(-1,1);

            // btConeTwistConstraint *joint = new btConeTwistConstraint(*rb1, *rb2, localA, localB);
            // joint->setLimit(0,0,0);
            physics->addConstraint(joint, true);
        }
    }
}

void AF::Activate(const btDynamicsWorld * physics)
{
    isActive_ = true;
    for (int i=0; i<17; i++)
    {
        physics->getCollisionObjectArray()[i+id_]->activate();
    }
}

btVector3 solve(btVector3 p, float r1, float r2, btVector3 dir)
{
    btVector3 q = p*( 0.5f + 0.5f*(r1*r1-r2*r2)/btDot(p,p) );

    float s = r1*r1 - btDot(q,q);
    s = btMax( s, 0.0f );
    q += sqrt(s)*p.cross(dir).normalized();

    return q;
}

#if 0
bool IkRigNode::SolveSpine(btVector3 & b, btVector3 & c, float l1, float l2, float l3)
{
    c = - tipOrigin.normalized();
    b = solve(c, l1, l2, axisDir);
    c = solve(tipOrigin, l2, l3, axisDir);
    return true;
}
#endif

void AF::UpdatePose()
{
    btVector3 b, c;
    btTransform hipsXform;
    IkRigNode spineNode;

    aJointOrigin_[0] = hipsXform.getOrigin();
    aJointOrigin_[1] = hipsXform.getOrigin() + b;
    aJointOrigin_[2] = hipsXform.getOrigin() + c;
    aJointOrigin_[3] = hipsXform.getOrigin() + spineNode.tipOrigin;

    btVector3 d = spineNode.tipOrigin;

    myArray<btMatrix3x3> aJointAxisMod_;
    myArray<btVector3> aJointOriginLocal_;
    aJointAxisMod_[0] = hipsXform.getBasis();
    aJointAxisMod_[1] = btMatrix3x3(shortestArcQuat(b, aJointOriginLocal_[1]));
    aJointAxisMod_[2] = btMatrix3x3(shortestArcQuat(c-b, aJointOriginLocal_[2]));
    aJointAxisMod_[3] = btMatrix3x3(shortestArcQuat(d-c, aJointOriginLocal_[3]));
}
