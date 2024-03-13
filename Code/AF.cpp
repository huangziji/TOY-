#include <btBulletDynamicsCommon.h>

class AF
{
    bool isActive;
public:
    bool LoadFile(const char *filename);
    bool SaveFile(const char *filename);
};

bool AF::LoadFile(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        return false;
    }

    const char *format = "%*[^,],";
    for (;;)
    {
        long cur = ftell(file);
        int a,b,c,d,e,f,g,h;
        int eof = fscanf(file, format, &a,&b,&c,&d,&e,&f,&g,&h);
        if (eof < 0) break;
        fseek(file, cur, SEEK_SET);
        fscanf(file, "%*[^\n]\n");
    }

    btDynamicsWorld *physics; // system
    btVector3 pivot, bodyOrigin, halfExtend;

    float mass = 1.0;
    btVector3 inertia;
    btTransform pose, dummy;
    pose.setIdentity();
    dummy.setIdentity();
    pose.setOrigin(bodyOrigin);
    btCollisionShape *shape = new btBoxShape(halfExtend);
    shape->calculateLocalInertia(mass, inertia);
    btRigidBody *rb = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
        mass, new btDefaultMotionState(pose, dummy), shape, inertia));
    physics->addRigidBody(rb);

    btRigidBody *rb1, *rb2;
    btTransform localA, localB;
    localA.setIdentity();
    localB.setIdentity();
    localA.setOrigin(pivot - rb1->getWorldTransform().getOrigin());
    localB.setOrigin(pivot - rb2->getWorldTransform().getOrigin());
    btGeneric6DofConstraint *joint = new btGeneric6DofConstraint(
            *rb1, *rb2, localA, localB, false);
    physics->addConstraint(joint);
    return true;
}

void Clear()
{
    btDynamicsWorld *physics;
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
