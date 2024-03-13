#include "Debug.h"

#include <stdio.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
using namespace glm;

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

    GLFWwindow *window1;
    {
        window1 = glfwCreateWindow(16*50, 9*50, "#1", NULL, NULL);
        glfwMakeContextCurrent(window1);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    }

    GLuint bufferA, tex5;
    {
        const int size = 9*50;
        glGenTextures(1, &tex5);
        glBindTexture(GL_TEXTURE_2D, tex5);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, size, size);
        glGenFramebuffers(1, &bufferA);
        glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex5, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
    }

    GLuint vbo1; glGenBuffers(1, &vbo1);
    const GLuint tex4 = loadTexture1("../Data/arial.bmp");

    myGui gui;
    gui.loadFnt("../Data/arial.fnt");

    while (!glfwWindowShouldClose(window1))
    {
        int iResolutionX, iResolutionY;
        double iMouseX, iMouseY;
        float iTime = glfwGetTime();
        float iTimeDelta;
        static uint32_t iFrame = -1;
        glfwGetFramebufferSize(window1, &iResolutionX, &iResolutionY);
        glfwGetCursorPos(window1, &iMouseX, &iMouseY);
        static float lastFrameTime = 0;
        iTimeDelta = iTime - lastFrameTime;
        lastFrameTime = iTime;
        iFrame += 1;

        static float fps = 0;
        if ((iFrame & 0xf) == 0) fps = 1. / iTimeDelta;

        typedef void (plugin)(void);
        void *f = loadPlugin("libToyxx_Plugin.so");
        if (f) ((plugin*)f)();

        gui.clearQuads();
        vec2 p = vec2(iResolutionX, iResolutionY) * vec2(.15,.9);
        gui.draw2dText(p, 20, "%.2f   %.1f fps   %dx%d", iTime, fps, iResolutionX, iResolutionY);
        p = vec2(iResolutionX, iResolutionY) * vec2(.4,.5);
        gui.drawRectangle(vec4(p,iResolutionX*.2,iResolutionY*.1),
                          vec4(0,0,iResolutionY,iResolutionY), vec4(1,0,1,1));
        gui.draw2dText(p, 30, "Hello, World!");

        myArray<float> const& V1 = gui.getQuadBuffer();

        glBindBuffer(GL_ARRAY_BUFFER, vbo1);
        glBufferData(GL_ARRAY_BUFFER, sizeof V1[0] * V1.size(), &V1[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 40, 0);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 40, (void*)16);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 40, (void*)32);
        glVertexAttribDivisor(1, 1);
        glVertexAttribDivisor(2, 1);
        glVertexAttribDivisor(3, 1);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, tex4);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, tex5);

        static long lastMod4;
        static const GLuint prog4 = glCreateProgram();
        bool dirty = loadShader1(&lastMod4, prog4, "../Data/Bake2.frag");
        if (dirty)
        {
            const int size = iResolutionY;
            glBindFramebuffer(GL_FRAMEBUFFER, bufferA);
            glViewport(0, 0, size, size);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(prog4);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        static long lastMod3;
        static const GLuint prog3 = glCreateProgram();
        loadShader2(&lastMod3, prog3, "../Data/Gui.glsl");
        glProgramUniform2f(prog3, 0, iResolutionX, iResolutionY);

        glClearColor(.5,.5,.5,1);
        glDepthMask(1);
        glFrontFace(GL_CCW);
        glDepthFunc(GL_LESS);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, iResolutionX, iResolutionY);
        glClear(GL_COLOR_BUFFER_BIT);

        glDepthMask(0);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(prog3);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, V1.size()/8);

        glfwSwapBuffers(window1);
        glfwPollEvents();
    }

    int err = glGetError();
    if (err) fprintf(stderr, "ERROR: %x\n", err);
}
