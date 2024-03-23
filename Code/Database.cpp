#include <stdio.h>
#include <string.h>
#include <ofbx.h>
#include <LinearMath/btHashMap.h>
#include <LinearMath/btQuickprof.h>
#include <LinearMath/btMatrix3x3.h>
using namespace ofbx;
template<class T> using myArray = btAlignedObjectArray<T>;

struct IkRigNode
{
    btMatrix3x3 tipAxis;
    btVector3 tipOrigin, axisDir;
    float length;

    IkRigNode Scaled(IkRigNode const& b) const
    {
        IkRigNode c;
        c.tipAxis = tipAxis.timesTranspose(b.tipAxis);
        c.tipOrigin = (tipOrigin - b.tipOrigin) / (length - b.length);
        c.axisDir = axisDir.cross(c.tipOrigin).normalized();
        return c;
    }
};

myArray<IkRigNode> &operator<<(myArray<IkRigNode> &a, IkRigNode b)
{
    a.push_back(b);
    return a;
}

myArray<IkRigNode> &operator,(myArray<IkRigNode> &a, IkRigNode b)
{
    return a << b;
}

static float Length(Vec3 a)
{
    return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
}

static Matrix operator*(const Matrix& lhs, const Matrix& rhs)
{
    Matrix res;
    for (int j = 0; j < 4; ++j)
    {
        for (int i = 0; i < 4; ++i)
        {
            double tmp = 0;
            for (int k = 0; k < 4; ++k)
            {
                tmp += lhs.m[i + k * 4] * rhs.m[k + j * 4];
            }
            res.m[i + j * 4] = tmp;
        }
    }
    return res;
}

static void SamplePoseRecursive(btHashMap<btHashString, IkRigNode> * out,
        float t, const IScene *scene,
        const Object *node, float length, Matrix parentWorld)
{
    Vec3 translation = node->getLocalTranslation();
    Vec3 rotation = node->getLocalRotation();
    const AnimationLayer *layer = scene->getAnimationStack(0)->getLayer(0);
    const AnimationCurveNode *channel1 = layer->getCurveNode(*node, "Lcl Translation");
    const AnimationCurveNode *channel2 = layer->getCurveNode(*node, "Lcl Rotation");
    if (channel1)
    {
        translation = channel1->getNodeLocalTransform(t);
    }
    if (channel2)
    {
        rotation = channel2->getNodeLocalTransform(t);
    }

    length += Length(node->getLocalTranslation());
    Matrix world = parentWorld * node->evalLocal(translation, rotation);

    IkRigNode * ikNode = out->find(node->name);
    if (ikNode)
    {
        ikNode->length = length;
        ikNode->tipOrigin = btVector3(world.m[12], world.m[13], world.m[14]);
        ikNode->axisDir = ikNode->tipOrigin - btVector3(parentWorld.m[12], parentWorld.m[13], parentWorld.m[14]);
        ikNode->tipAxis.setValue(
                world.m[0],world.m[1],world.m[2],
                world.m[4],world.m[5],world.m[6],
                world.m[9],world.m[8],world.m[10] );
    }

    for (int i=0; node->resolveObjectLink(i); i++)
    {
        const Object *child = node->resolveObjectLink(i);
        if (child->isNode()) SamplePoseRecursive(out, t, scene, child, length, world);
    }
}

const IScene * loadFbx(const char *filename)
{
    btClock stop;
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    u8 data[length];
    fread(data, 1, length, f);
    fclose(f);

    IScene *fbxScene = load(data, length, 0);
    const char *err = getError();
    if (strlen(err)) fprintf(stderr, "ERROR: %s\n", err);

    printf("INFO: loaded file %s. It took %ld ms\n", filename, stop.getTimeMilliseconds());
    return fbxScene;
}

myArray<IkRigNode> SamplePose(float t, const IScene *scene)
{
    static const char *KEYWORDS[] = {
        "mixamorig:Hips",
        "mixamorig:Neck",
        "mixamorig:Head",
        "mixamorig:LeftUpLeg",
        "mixamorig:LeftFoot",
        "mixamorig:LeftShoulder",
        "mixamorig:LeftHand",
        "mixamorig:RightUpLeg",
        "mixamorig:RightFoot",
        "mixamorig:RightShoulder",
        "mixamorig:RightHand",
    };

    btHashMap<btHashString, IkRigNode> out;
    for (const char *key : KEYWORDS)
    {
        out.insert(key, {});
    }

    const Matrix identity = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
    SamplePoseRecursive(&out, t, scene, scene->getRoot(), 0, identity);

    const IkRigNode * a = out.find(KEYWORDS[0]);
    const IkRigNode * b = out.find(KEYWORDS[1]);
    const IkRigNode * c = out.find(KEYWORDS[2]);
    const IkRigNode * d = out.find(KEYWORDS[3]);
    const IkRigNode * e = out.find(KEYWORDS[4]);
    const IkRigNode * f = out.find(KEYWORDS[5]);
    const IkRigNode * g = out.find(KEYWORDS[6]);
    const IkRigNode * h = out.find(KEYWORDS[7]);
    const IkRigNode * i = out.find(KEYWORDS[8]);
    const IkRigNode * j = out.find(KEYWORDS[9]);
    const IkRigNode * k = out.find(KEYWORDS[10]);

    myArray<IkRigNode> res;
    res << a->Scaled({}), b->Scaled(*a), c->Scaled(*b),
        e->Scaled(*d), g->Scaled(*f), i->Scaled(*h), k->Scaled(*j);
    return res;
}

float GetAnimDuration(const IScene *scene)
{
    const AnimationLayer *layer = scene->getAnimationStack(0)->getLayer(0);
    for (int i=0; layer->getCurveNode(i); i++)
    {
        const AnimationCurveNode *channel = layer->getCurveNode(i);
        if (channel)
        {
            int n = channel->getCurve(0)->getKeyCount();
            float f = channel->getCurve(0)->getKeyTime()[n - 1];
            return fbxTimeToSeconds(f);
        }
    }
    return 0;
}

btVector3 GetAvgVelocityRM(const IScene *scene)
{
    const AnimationLayer *layer = scene->getAnimationStack(0)->getLayer(0);
    for (int i=0; layer->getCurveNode(i); i++)
    {
        const AnimationCurveNode *channel = layer->getCurveNode(i);
        if (channel->name[0] == 'T')
        {
            const float d = GetAnimDuration(scene);
            const float l = Length(channel->getBone()->getLocalTranslation());
            Vec3 p = channel->getNodeLocalTransform(0);
            Vec3 q = channel->getNodeLocalTransform(d);
            btVector3 avgVel = btVector3(q.x-p.x, q.y-p.y, q.z-p.z) / (d * l);
            return avgVel;
        }
    }
    return {};
}
