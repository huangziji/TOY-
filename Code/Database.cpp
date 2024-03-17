#include <stdio.h>
#include <string.h>
#include <ofbx.h>
#include <LinearMath/btQuickprof.h>
#include <LinearMath/btHashMap.h>
#include <LinearMath/btVector3.h>
using namespace ofbx;

struct IkRigNode
{
    btVector3 world, local; float length;

    btVector3 GetScaledT(IkRigNode const& other) const
    {
        btVector3 t = (world - other.world) / (length - other.length);
        btVector3 s = local.cross(t).normalized();
        return t;
    }
};

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
        ikNode->world = btVector3(world.m[12], world.m[13], world.m[14]);
        ikNode->local = ikNode->world - btVector3(parentWorld.m[12], parentWorld.m[13], parentWorld.m[14]);
    }

    for (int i=0; node->resolveObjectLink(i); i++)
    {
        const Object *child = node->resolveObjectLink(i);
        if (child->isNode()) SamplePoseRecursive(out, t, scene, child, length, world);
    }
}

const IScene * loadFbx(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return NULL;
    }

    btClock stop;
    stop.reset();

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    u8 data[length];
    fread(data, 1, length, f);
    fclose(f);

    IScene *fbxScene = load(data, length, 0);
    const char *err = getError();
    if (strlen(err)) fprintf(stderr, "ERROR: %s\n", err);
    unsigned long long elapsedTime = stop.getTimeMilliseconds();
    printf("INFO: loaded file %s. It took %ld ms\n", filename, elapsedTime);

    return fbxScene;
}

void SamplePose(float t, const IScene *scene)
{
    static const char *KEYWORDS[] = {
        "mixamorig:Hips",
        "mixamorig:Neck",
        "mixamorig:Head",
        "mixamorig:LeftFoot",
        "mixamorig:LeftHand",
        "mixamorig:LeftUpLeg",
        "mixamorig:LeftShoulder",
        "mixamorig:RightFoot",
        "mixamorig:RightHand",
        "mixamorig:RightUpLeg",
        "mixamorig:RightShoulder",
    };

    btHashMap<btHashString, IkRigNode> out;
    for (const char *key : KEYWORDS)
    {
        out.insert(key, {});
    }

    const Matrix identity = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
    SamplePoseRecursive(&out, t, scene, scene->getRoot(), 0, identity);

    const IkRigNode * hips = out.find(KEYWORDS[0]);
    const IkRigNode * neck = out.find(KEYWORDS[1]);
    const IkRigNode * head = out.find(KEYWORDS[2]);
    const IkRigNode * leftFoot = out.find(KEYWORDS[3]);
    const IkRigNode * leftHand = out.find(KEYWORDS[4]);
    const IkRigNode * leftUpLeg = out.find(KEYWORDS[5]);
    const IkRigNode * leftShoulder = out.find(KEYWORDS[6]);
    const IkRigNode * rightFoot = out.find(KEYWORDS[7]);
    const IkRigNode * rightHand = out.find(KEYWORDS[8]);
    const IkRigNode * rightUpLeg = out.find(KEYWORDS[9]);
    const IkRigNode * rightShoulder = out.find(KEYWORDS[10]);

    btVector3 a = neck->GetScaledT({});
    btVector3 b = neck->GetScaledT(*hips);
    btVector3 c = head->GetScaledT(*neck);
    btVector3 d = leftFoot->GetScaledT(*leftUpLeg);
    btVector3 e = leftHand->GetScaledT(*leftShoulder);
    btVector3 f = rightFoot->GetScaledT(*rightUpLeg);
    btVector3 g = rightHand->GetScaledT(*rightShoulder);
    btVector3 h = neck->local.cross(b).normalized();
    btVector3 i = head->local.cross(c).normalized();
    btVector3 j = leftFoot->local.cross(d).normalized();
    btVector3 k = leftHand->local.cross(e).normalized();
    btVector3 l = rightFoot->local.cross(f).normalized();
    btVector3 m = rightHand->local.cross(g).normalized();
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
