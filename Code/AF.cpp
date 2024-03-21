#include "AF.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
using namespace glm;

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
        halfExtents = ((btBoxShape*)rb.getCollisionShape())->getHalfExtentsWithoutMargin();
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

btRigidBody *createRigidBody(btVector3 origin, btVector3 bounds)
{
    float rz = 0;
    int shapeType = BOX_SHAPE_PROXYTYPE;
    btCollisionShape *shape = NULL;
    switch (shapeType) {
    case BOX_SHAPE_PROXYTYPE:
        shape = new btBoxShape(bounds);
        break;
    case CAPSULE_SHAPE_PROXYTYPE:
        shape = new btCapsuleShape(bounds[0], bounds[1]);
        break;
    }

    float mass = 1.0;
    btVector3 inertia;
    shape->calculateLocalInertia(mass, inertia);
    btRigidBody *rb = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
        mass, NULL, shape, inertia));
    btTransform x;
    x.setIdentity();
    x.setOrigin(origin);
    x.getBasis().setEulerZYX(0,0,rz);
    rb->setWorldTransform(x);
    rb->setActivationState(btRigidBody::CF_KINEMATIC_OBJECT);
    return rb;
}

void AF::Load(btDynamicsWorld *physics)
{
    const int X[][12] = {
         1,-1,    0,100,0,   15,10,10,  -1,-1,-1,  0,
         2, 0,    0,120,0,   15,10,10,  -1,-1,-1,  0,
         3, 1,    0,140,0,   15,10,10,  -1,-1,-1,  0,
         4, 2,    0,160,0,    3, 3, 3,  -1,-1,-1,  0,
         0, 3,    0,163,0,   10, 2, 0,   0,185,0,  0,

         6, 0,   10,100,0,    7,35, 0,  -1,-1,-1,  0,
         7, 5,   10, 50,0,    7,35, 0,  -1,-1,-1,  0,
         0, 6,   10,  0,0,    5, 5, 8,  10, 0, 8,  0,
         9, 2,   15,155,0,    5,21, 0,  -1,-1,-1,  90,
        10, 8,   45,155,0,    5,21, 0,  -1,-1,-1,  90,
         0, 9,   75,155,0,    6, 2, 0,  90,155,0,  90,

        12, 0,  -10,100,0,    7,35, 0,  -1,-1,-1,  0,
        13,11,  -10, 50,0,    7,35, 0,  -1,-1,-1,  0,
         0,12,  -10,  0,0,    5, 5, 8,  -10,0, 8,  0,
        15, 2,  -15,155,0,    5,21, 0,  -1,-1,-1,  90,
        16,14,  -45,155,0,    5,21, 0,  -1,-1,-1,  90,
         0,15,  -75,155,0,    6, 2, 0, -90,155,0,  90,
    };

    isActive_ = false;
    id_ = physics->getNumCollisionObjects();
    aJointOrigin_.clear();

    for (int i=0; i<countof(X); i++)
    {
        int a = X[i][0], b = X[i][1], c = X[i][2],
            d = X[i][3], e = X[i][4], f = X[i][5],
            g = X[i][6], h = X[i][7], j = X[i][11],
            k = a ? X[a][2] : X[i][8],
            l = a ? X[a][3] : X[i][9],
            m = a ? X[a][4] : X[i][10];

        bool isBox = h > 0;
        btVector3 bounds = btVector3(f,g,h) * 0.01f;
        btVector3 jointOrigin1 = btVector3(c,d,e) * 0.01f;
        btVector3 jointOrigin2 = btVector3(k,l,m) * 0.01f;
        btVector3 bodyOrigin = (jointOrigin1 + jointOrigin2) * 0.5f;

        btCollisionShape *shape = isBox ?
            (btCollisionShape *)new btBoxShape(bounds) :
            (btCollisionShape *)new btCapsuleShape(bounds.x(), bounds.y());
        float mass = 1.0;
        btVector3 inertia;
        shape->calculateLocalInertia(mass, inertia);
        btRigidBody *rb1 = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
            mass, NULL, shape, inertia));
        btTransform x;
        x.setIdentity();
        x.setOrigin(bodyOrigin);
        x.getBasis().setEulerZYX(0, 0, btRadians(j));
        rb1->setWorldTransform(x);
        rb1->setActivationState(btRigidBody::CF_KINEMATIC_OBJECT);
        physics->addRigidBody(rb1);

        if (i)
        {
            btRigidBody *rb2 = btRigidBody::upcast(physics->getCollisionObjectArray()[id_ + b]);
            btTransform localA;
            btTransform localB;
            localA.setIdentity();
            localB.setIdentity();
            localA.setOrigin( rb1->getWorldTransform().invXform(jointOrigin1) );
            localB.setOrigin( rb2->getWorldTransform().invXform(jointOrigin1) );
            btGeneric6DofConstraint *joint = new btGeneric6DofConstraint(
                        *rb1, *rb2, localA, localB, false);
            joint->setLimit(0,0,0);
            joint->setLimit(1,0,0);
            joint->setLimit(2,0,0);
            joint->setLimit(3,0,SIMD_PI*.25);
            joint->setLimit(4,0,SIMD_PI*.25);
            joint->setLimit(5,0,SIMD_PI*.15);
            physics->addConstraint(joint, true);
        }
    }
}

void AF::Activate(const btDynamicsWorld * physics)
{
    isActive_ = true;
    for (int i=0; i<17; i++)
    {
        btRigidBody::upcast(physics->getCollisionObjectArray()[i+id_])->activate();
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

struct IkRigNode
{
    btMatrix3x3 tipAxis;
    btVector3 tipOrigin, axisDir;
    float length;

    bool SolveSpine(btVector3 & b, btVector3 & c, float l1, float l2, float l3);
};

bool IkRigNode::SolveSpine(btVector3 & b, btVector3 & c, float l1, float l2, float l3)
{
    c = - tipOrigin.normalized();
    b = solve(c, l1, l2, axisDir);
    c = solve(tipOrigin, l2, l3, axisDir);
    return true;
}

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
