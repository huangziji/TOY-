#include "AF.h"
#include <sys/stat.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
template <typename T> using myArray = btAlignedObjectArray<T>;
using namespace glm;

myArray<float> &operator<<(myArray<float> &a, float b)
{
    a.push_back(b);
    return a;
}

myArray<float> &operator,(myArray<float> &a, float b)
{
    return a << b;
}

myArray<float> &operator<<(myArray<float> &a, vec4 b)
{
    return a << b.x, b.y, b.z, b.w;
}

myArray<float> &operator,(myArray<float> &a, vec4 b)
{
    return a << b;
}

myArray<float> &operator<<(myArray<float> &a, mat4 b)
{
    return a << b[0], b[1], b[2], b[3];
}

void Clear(btDynamicsWorld *physics)
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

mat4 drawPrimitive( vec3 origin, mat3 axis, vec3 halfExtend, int shapeType )
{
    mat4 data = axis;
    data[0][3] = origin.x, data[1][3] = origin.y, data[2][3] = origin.z;
    data[3] = vec4(halfExtend, intBitsToFloat(shapeType));
    return data;
}

mat4 drawRigidBody( btCollisionObject const& rb )
{
    vec3 h;
    int shapeType;
    switch (rb.getCollisionShape()->getShapeType())
    {
    case STATIC_PLANE_PROXYTYPE:
    {
        h.x = -dynamic_cast<const btStaticPlaneShape*>(rb.getCollisionShape())->getPlaneConstant();
        shapeType = 3;
        break;
    }
    case BOX_SHAPE_PROXYTYPE:
    {
        btVector3 halfExtents = dynamic_cast<const btBoxShape*>(rb.getCollisionShape())->getHalfExtentsWithMargin();
        h.x = halfExtents.x();
        h.y = halfExtents.y();
        h.z = halfExtents.z();
        shapeType = 1;
        break;
    }
    case CAPSULE_SHAPE_PROXYTYPE:
        h.x = dynamic_cast<const btCapsuleShape*>(rb.getCollisionShape())->getHalfHeight();
        h.y = dynamic_cast<const btCapsuleShape*>(rb.getCollisionShape())->getRadius();
        shapeType = 2;
        break;
    }

    btVector3 origin = rb.getWorldTransform().getOrigin();
    btMatrix3x3 axis = rb.getWorldTransform().getBasis().transpose();
    return drawPrimitive(vec3(origin[0], origin[1], origin[2]),
        mat3( axis[0][0], axis[0][1], axis[0][2],
              axis[1][0], axis[1][1], axis[1][2],
              axis[2][0], axis[2][1], axis[2][2] ),
        h, shapeType);
}

int loadAF(AF *self, btDynamicsWorld *physics)
{
    const int AF_DATA[][12] = {
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

    self->aJointParentIndex_.clear();
    self->aAtRestOrigin_.clear();
    self->isActive_ = true;
    self->id_ = physics->getNumCollisionObjects();

    for (int i=0; i<countof(AF_DATA); i++)
    {
        int a = AF_DATA[i][0],
            b = AF_DATA[i][1],
            c = AF_DATA[i][2],
            d = AF_DATA[i][3],
            e = AF_DATA[i][4],
            f = AF_DATA[i][5],
            g = AF_DATA[i][6],
            h = AF_DATA[i][7],
            j = AF_DATA[i][11];

        int x = a ? AF_DATA[a][2] : AF_DATA[i][8],
            y = a ? AF_DATA[a][3] : AF_DATA[i][9],
            z = a ? AF_DATA[a][4] : AF_DATA[i][10];

        bool isBox = h > 0;
        btVector3 bounds = btVector3(f,g,h) * 0.01f;
        btVector3 jointOrigin1 = btVector3(c,d,e) * 0.01f;
        btVector3 jointOrigin2 = btVector3(x,y,z) * 0.01f;
        btVector3 bodyOrigin = (jointOrigin1 + jointOrigin2) * 0.5f;

        btTransform xform, _dummy;
        xform.setIdentity();
        _dummy.setIdentity();
        xform.setOrigin(bodyOrigin);
        xform.getBasis().setEulerYPR(btRadians(j), 0, 0);

        float mass = 1.0;
        btVector3 inertia;
        btCollisionShape *shape = isBox ?
            (btCollisionShape *)new btBoxShape(bounds) :
            (btCollisionShape *)new btCapsuleShape(bounds.x(), bounds.y());
        shape->calculateLocalInertia(mass, inertia);
        btRigidBody *rb1 = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
            mass, new btDefaultMotionState(xform, _dummy), shape, inertia));
        physics->addRigidBody(rb1);

        if (i)
        {
            btRigidBody *rb2 = btRigidBody::upcast(physics->getCollisionObjectArray()[self->id_ + b]);
            btTransform localA;
            btTransform localB;
            localA.setIdentity();
            localB.setIdentity();
            localA.setOrigin( rb1->getWorldTransform().inverse() * jointOrigin1 );
            localB.setOrigin( rb2->getWorldTransform().inverse() * jointOrigin1 );
            btGeneric6DofConstraint *joint = new btGeneric6DofConstraint(
                        *rb1, *rb2, localA, localB, false);
            physics->addConstraint(joint, true);
            joint->setLimit(0,0,0);
            joint->setLimit(1,0,0);
            joint->setLimit(2,0,0);
            joint->setLimit(3,0,SIMD_PI*.25);
            joint->setLimit(4,0,SIMD_PI*.25);
            joint->setLimit(5,0,SIMD_PI*.15);
        }
    }
    return 0;
}

void deactivateAF(AF *self, const btDynamicsWorld * physics)
{
    self->isActive_ = false;
    for (int i=0; i<17; i++)
    {
        physics->getCollisionObjectArray()[i+self->id_]->
                setActivationState(btRigidBody::CF_KINEMATIC_OBJECT);
    }
}

btVector3 solve(btVector3 p, float r1, float r2, btVector3 dir)
{
    btVector3 q = p*( 0.5f + 0.5f*(r1*r1-r2*r2)/btDot(p,p) );

    float s = r1*r1 - btDot(q,q);
    s = max( s, 0.0f );
    q += sqrt(s)*btCross(p,dir).normalized();

    return q;
}

void AFUpdatePose(myArray<btVector3> const& ikNode)
{
    float  l1, l2;
    btVector3 b, c, d, v;
    c = -d.normalized();
    b = solve(c, l1, l2, v);
    c = solve(d, l1, l2, v);
}
