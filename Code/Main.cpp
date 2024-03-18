#include "Debug.h"

#include <stdio.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
using namespace glm;

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

bool loadShader1(long *lastMod, GLuint prog, const char *filename);
bool loadShader2(long *lastMod, GLuint prog, const char *filename);
void *loadPlugin(const char *filename);
GLuint loadTexture1(const char *filename);

#include <sys/stat.h>

bool loadShader3(long *lastMod, GLuint prog, const char *filename)
{
    struct stat libStat;
    int err = stat(filename, &libStat);
    if (err || *lastMod == libStat.st_mtime)
    {
        return false;
    }

    printf("INFO: reloading file %s\n", filename);
    *lastMod = libStat.st_mtime;

    FILE *f = fopen(filename, "r");
    assert(f);
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char source[length+1]; source[length] = 0; // set null terminator
    fread(source, length, 1, f);
    fclose(f);

    const char *string[] = { source };
    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, 1, string, NULL);
    glCompileShader(cs);
    int success;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        int length;
        glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &length);
        char message[length];
        glGetShaderInfoLog(cs, length, &length, message);
        glDeleteShader(cs);
        fprintf(stderr, "ERROR: fail to compile compute shader. file %s\n%s\n",
                filename, message);
        return true;
    }

    GLsizei NbShaders;
    GLuint oldShader;
    glGetAttachedShaders(prog, 1, &NbShaders, &oldShader);
    if (NbShaders)
    {
        glDetachShader(prog, oldShader);
        glDeleteShader(oldShader);
    }

    glAttachShader(prog, cs);
    glLinkProgram(prog);
    glValidateProgram(prog);
    return true;
}

#include <btBulletDynamicsCommon.h>
#include <cgltf.h>

static cgltf_data *load_cgltf(const char *filename)
{
    btClock stop;
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

    printf("INFO: loaded file %s. It took %d ms\n", filename, stop.getTimeMilliseconds());
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
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
        const int size = 9*50;
        glGenTextures(1, &tex5);
        glBindTexture(GL_TEXTURE_2D, tex5);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, size, size);
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, size, size, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
        glGenFramebuffers(1, &bufferA);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex5, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_NONE);
    }

    GLuint vbo1; glGenBuffers(1, &vbo1);
    GLuint vbo2; glGenBuffers(1, &vbo2);
    GLuint ubo1; glGenBuffers(1, &ubo1);
    GLuint ssbo1; glGenBuffers(1, &ssbo1);
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
        iMouseY = iResolutionY-iMouseY;
        static float lastFrameTime = 0;
        iTimeDelta = iTime - lastFrameTime;
        lastFrameTime = iTime;
        iFrame += 1;

        static btCollisionConfiguration *conf = new btDefaultCollisionConfiguration;
        static btDynamicsWorld *physics = new btDiscreteDynamicsWorld(
                    new btCollisionDispatcher(conf), new btDbvtBroadphase,
                    new btSequentialImpulseConstraintSolver, conf);

        typedef struct { myArray<float> U1,W1,V2; }Output;
        Output out;
        typedef void (plugin)(Output *, btDynamicsWorld *, float, float);
        void *f = loadPlugin("./libToyxx_Plugin.so");
        if (f) ((plugin*)f)(&out, physics, iTime, iTimeDelta);

        gui.clearQuads();
        static float fps = 0; if ((iFrame & 0xf) == 0) fps = 1. / iTimeDelta;
        gui.draw2dText(iResolutionX*.15, iResolutionY*.9, 0xFF7FFFFF, 20,
            "%.2f   %.1f fps   %dx%d", iTime, fps, iResolutionX, iResolutionY);
        gui.drawRectangle({ iResolutionX*.1, iResolutionY*.9, iResolutionY*.05,iResolutionY*.05 },
                          { 0, 0, iResolutionY*.5, iResolutionY*.5 }, 0xFF7FFFFF);

        myArray<int> const& V1 = gui.Data();
        myArray<float> const& V2 = out.V2;
        myArray<float> const& U1 = out.U1;
        myArray<float> const& W1 = out.W1;

        glBindBuffer(GL_ARRAY_BUFFER, vbo1);
        glBufferData(GL_ARRAY_BUFFER, sizeof V1[0] * V1.size(), &V1[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribIPointer(1, 4, GL_INT, 32, 0);
        glVertexAttribIPointer(2, 4, GL_INT, 32, (void*)16);
        glVertexAttribDivisor(1, 1);
        glVertexAttribDivisor(2, 1);

        glBindBuffer(GL_ARRAY_BUFFER, vbo2);
        glBufferData(GL_ARRAY_BUFFER, sizeof V2[0] * V2.size(), &V2[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 16, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo1);
        glBufferData(GL_UNIFORM_BUFFER, sizeof U1[0] * U1.size(), &U1[0], GL_DYNAMIC_COPY);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo1);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo1);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof W1[0] * W1.size(), &W1[0], GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo1);

        static long lastMod4;
        static const GLuint prog4 = glCreateProgram();
        bool dirty = loadShader1(&lastMod4, prog4, "../Data/Bake2.frag");
        if (dirty)
        {
            const int size = iResolutionY;
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
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, V1.size()/8);

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
        ImGui::Begin("Hello World");
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window1);
        glfwPollEvents();
    }

    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
}
