#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <LinearMath/btAlignedObjectArray.h>
template <typename T> using myArray = btAlignedObjectArray<T>;

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

bool loadShader1(long *lastMod, GLuint prog, const char *filename);
bool loadShader2(long *lastMod, GLuint prog, const char *filename);
bool loadShader3(long *lastMod, GLuint prog, const char *filename);
void *loadPlugin(const char *filename);
GLuint loadTexture1(const char *filename);
bool screenRecording();

typedef struct {int x,y,z,w; }int4;
int4 Int4(int x, int y, int z, int w) { return { x,y,z,w }; }

class myGui
{
    typedef struct{ int x,y,w,h,xoff,yoff,xadv; }Glyph;
    myArray<Glyph> _fntInfo;
    myArray<int> _quadBuffer;
public:
    myArray<int> const& Data() const { return _quadBuffer; }
    void clearQuads() { _quadBuffer.clear(); }
    bool loadFnt(const char *filename);
    void draw2dText(int posX, int posY, int fntSize, int col, const char *fmt...);
    void drawRectangle(int4 const& dst, int4 const& src, int col, int texSlot = 1);
};

myArray<int> &operator<<(myArray<int> &a, int b)
{
    a.push_back(b);
    return a;
}

myArray<int> &operator,(myArray<int> &a, int b)
{
    return a << b;
}

void myGui::drawRectangle(int4 const& dst, int4 const& src, int col, int texSlot)
{
    _quadBuffer <<
            (dst.x | (dst.y << 16)), (dst.z | (dst.w << 16)),
            (src.x | (src.y << 16)), (src.z | (src.w << 16)),
            col, texSlot;
}

bool myGui::loadFnt(const char *filename)
{
    const time_t stop = clock();
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return false;
    }

    _fntInfo.resize(128);
    const char *format =
            "%*[^=]=%d%*[^=]=%d%*[^=]=%d%*[^=]=%d"
            "%*[^=]=%d%*[^=]=%d%*[^=]=%d%*[^=]=%d";
    fscanf(file, "%*[^\n]\n%*[^\n]\n%*[^\n]\n%*[^\n]\n");
    for (;;)
    {
        long cur = ftell(file);
        int a,b,c,d,e,f,g,h;
        int eof = fscanf(file, format, &a,&b,&c,&d,&e,&f,&g,&h);
        if (eof < 0) break;
        fseek(file, cur, SEEK_SET);
        fscanf(file, "%*[^\n]\n");
        _fntInfo[a] = { b,c,d,e,f,g,h };
    }

    fclose(file);
    const time_t elapseTime = (clock()-stop) / 1000;
    printf("INFO: loaded file %s. It took %d ms\n", filename, elapseTime);
    return true;
}

void myGui::draw2dText(int posX, int posY, int fntSize, int col, const char *format...)
{
    char textString[128];
    va_list args;
    va_start(args, format);
    vsprintf(textString, format, args);
    va_end(args);

    const float spacing = 0;
    const int nChars = strlen(textString);
    const int charWid = _fntInfo['_'].xadv;
    const int lineHei = charWid * 2;

    for (int x=0, y=0, i=0; i<nChars; i++)
    {
        int code = textString[i];
        if (code == '\n')
        {
            x = 0;
            y += fntSize;
            continue;
        }

        Glyph g = _fntInfo[code];
        int4 dst = {
            g.xoff * fntSize / lineHei + x + posX,
            g.yoff * fntSize / lineHei + y + posY,
            g.w * fntSize / lineHei,
            g.h * fntSize / lineHei, };
        drawRectangle(dst, { g.x,g.y,g.w,g.h }, col, 0);

        int xadv = code == ' ' ? charWid : g.xadv;
        x += xadv * fntSize / lineHei + spacing;
    }
}

#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>
#include <cgltf.h>

static cgltf_data *load_cgltf(const char *filename)
{
    const time_t stop = clock();
    cgltf_data* data = NULL;
    cgltf_options options = {};
    cgltf_result result;

    result = cgltf_parse_file(&options, filename, &data);
    if (result != cgltf_result_success) {
        return 0;
    }
    result = cgltf_load_buffers(&options, data, filename);
    if (result != cgltf_result_success) {
        cgltf_free(data);
        return 0;
    }
    result = cgltf_validate(data);
    if (result != cgltf_result_success) {
        cgltf_free(data);
        return 0;
    }

    const time_t elapseTime = (clock() - stop) / 1000;
    printf("INFO: loaded file %s. It took %d ms\n", filename, elapseTime);
    return data;
}

static void error_callback(int _, const char* desc)
{
    fprintf(stderr, "ERROR: %s\n", desc);
}

int main()
{
    glfwInit();
    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    // glfwWindowHint(GLFW_FLOATING, GL_TRUE);

    GLFWwindow *window1;
    {
        const int RES_X = 16*50, RES_Y = 9*50;
        window1 = glfwCreateWindow(RES_X, RES_Y, "#1", NULL, NULL);
        glfwMakeContextCurrent(window1);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        // set window properties
        int screenWidth, screenHeight;
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        glfwGetMonitorWorkarea(primaryMonitor, NULL, NULL, &screenWidth, &screenHeight);
        glfwSetWindowPos(window1, screenWidth-RES_X, 0);
    }

    GLuint bufferA, tex5;
    {
        const int size = 450;
        glGenTextures(1, &tex5);
        glBindTexture(GL_TEXTURE_2D, tex5);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, size, size);
        glGenFramebuffers(1, &bufferA);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex5, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_NONE);
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, newSize, newSize, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    }

    GLuint vbo1; glGenBuffers(1, &vbo1);
    GLuint vbo2; glGenBuffers(1, &vbo2);
    GLuint ubo1; glGenBuffers(1, &ubo1);
    GLuint ssbo1; glGenBuffers(1, &ssbo1);
    GLuint vbo3; glGenBuffers(1, &vbo3);
    GLuint abo1; glGenBuffers(1, &abo1);
    myGui gui; gui.loadFnt("../Data/arial.fnt");
    const GLuint tex4 = loadTexture1("../Data/arial.bmp");

    while (!glfwWindowShouldClose(window1))
    {
        int iResolutionX, iResolutionY;
        double iMouseX, iMouseY;
        float iTime = glfwGetTime();
        float iTimeDelta;
        static uint32_t iFrame = -1;
        glfwGetWindowSize(window1, &iResolutionX, &iResolutionY);
        glfwGetCursorPos(window1, &iMouseX, &iMouseY);
        static float lastFrameTime = 0;
        iTimeDelta = iTime - lastFrameTime;
        lastFrameTime = iTime;
        iFrame += 1;

        static btSoftBodyRigidBodyCollisionConfiguration *conf = new btSoftBodyRigidBodyCollisionConfiguration;
        static btSoftRigidDynamicsWorld *physics = new btSoftRigidDynamicsWorld(
                    new btCollisionDispatcher(conf), new btDbvtBroadphase,
                    new btSequentialImpulseConstraintSolver, conf);

        typedef struct { myArray<float> U1,W1,V2; }Output;
        Output out;
        typedef void (plugin)(Output *, btDynamicsWorld *, float, float, float, float, float, float);
        void *f = loadPlugin("./libToyxx_Plugin.so");
        if (f) ((plugin*)f)(&out, physics, iTime, iTimeDelta, iResolutionX, iResolutionY, iMouseX, iMouseY);

        gui.clearQuads();
        static float fps = 0; if ((iFrame & 0xf) == 0) fps = 1. / iTimeDelta;
        gui.draw2dText(iResolutionX*.15, iResolutionY*.9,  20, 0xFF7FFFFF,
            "%.2f   %.1f fps   %dx%d", iTime, fps, iResolutionX, iResolutionY);
        gui.drawRectangle(Int4( iResolutionX*.1, iResolutionY*.9, iResolutionY*.05,iResolutionY*.05 ),
                          Int4( 0, 0, iResolutionY*.5, iResolutionY*.5 ), 0xFF7FFFFF);

        myArray<int> const& V1 = gui.Data();
        myArray<float> const& V2 = out.V2;
        myArray<float> const& U1 = out.U1;
        myArray<float> const& W1 = out.W1;

        glBindBuffer(GL_ARRAY_BUFFER, vbo1);
        glBufferData(GL_ARRAY_BUFFER, sizeof V1[0] * V1.size(), &V1[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribIPointer(1, 4, GL_INT, 24, 0);
        glVertexAttribIPointer(2, 2, GL_INT, 24, (void*)16);
        glVertexAttribDivisor(1, 1);
        glVertexAttribDivisor(2, 1);

        glBindBuffer(GL_ARRAY_BUFFER, vbo2);
        glBufferData(GL_ARRAY_BUFFER, sizeof V2[0] * V2.size(), &V2[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 16, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo1);
        glBufferData(GL_UNIFORM_BUFFER, sizeof U1[0] * U1.size(), &U1[0], GL_DYNAMIC_COPY);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo1);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo1);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof W1[0] * W1.size(), &W1[0], GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo1);

        static long lastMod4;
        static const GLuint prog4 = glCreateProgram();
        bool dirty = loadShader1(&lastMod4, prog4, "../Data/Bake2.frag");
        if (dirty)
        {
            const int size = 450;
            glDepthMask(0);
            glDisable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
            glViewport(0, 0, size, size);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(prog4);
            glProgramUniform2f(prog4, 0, size, size);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        static long lastMod5;
        static const GLuint prog5 = glCreateProgram();
        dirty = loadShader3(&lastMod5, prog5, "../Data/Bake3.glsl");
        if (dirty)
        {
            const int resX = 64, resY = 64, resZ = 64, length = 0;
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo3);
            glBufferData(GL_SHADER_STORAGE_BUFFER, 512, NULL, GL_STATIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo3);
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, abo1);
            glBufferData(GL_ATOMIC_COUNTER_BUFFER, 4, &length, GL_STATIC_COPY);
            glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, abo1);
            glProgramUniform3ui(prog5, 0, resX, resY, resZ);
            glUseProgram(prog5);
            glDispatchCompute(resX/8, resY/8, resZ/8);
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo3);
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 12, 0);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, tex4);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, tex5);

        glDepthMask(1);
        glFrontFace(GL_CCW);
        glDepthFunc(GL_LESS);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, iResolutionX, iResolutionY);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        static long lastMod1;
        static const GLuint prog1 = glCreateProgram();
        loadShader1(&lastMod1, prog1, "../Data/Base.frag");

        glProgramUniform2f(prog1, 0, iResolutionX, iResolutionY);
        glProgramUniform1i(prog1, 4, W1.size()/16);
        glUseProgram(prog1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        static long lastMod2;
        static const GLuint prog2 = glCreateProgram();
        loadShader2(&lastMod2, prog2, "../Data/Unlit.glsl");

        glDepthMask(0);
        glProgramUniform2f(prog2, 0, iResolutionX, iResolutionY);
        glUseProgram(prog2);
        glDrawArrays(GL_LINES, 0, V2.size()/4);

        static long lastMod3;
        static const GLuint prog3 = glCreateProgram();
        loadShader2(&lastMod3, prog3, "../Data/Gui.glsl");

        glDisable(GL_DEPTH_TEST);
        glProgramUniform2f(prog3, 0, iResolutionX, iResolutionY);
        glUseProgram(prog3);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, V1.size()/6);

        if (iFrame == 0)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            ImGui_ImplGlfw_InitForOpenGL(window1, true);
            ImGui_ImplOpenGL3_Init("#version 130");
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window1);
        glfwPollEvents();
        // if (!screenRecording()) break;
    }

    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
}
