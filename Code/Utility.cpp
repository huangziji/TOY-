#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include <glad/glad.h>
#include <stb_image.h>

/***************************************************************
 *                        Hotloading                           *
***************************************************************/

static int loadShaderS(int N, GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char version[32];
    fgets(version, sizeof(version), f);
    length -= ftell(f);
    char source[length+1]; source[length] = 0; // set null terminator
    fread(source, length, 1, f);
    fclose(f);

    const char vsSource[] = R"(
    precision mediump float;
    void main() {
        vec2 UV = vec2(gl_VertexID%2, gl_VertexID/2)*2.-1.;
        gl_Position = vec4(UV, 0, 1);
    }
    )";

    const char *string[] = {
        version, "#define _VS\n", N == 1 ? vsSource : source,
        version, "#define _FS\n", source, };

    GLuint newShader[2];
    for (int i=0; i<2; i++)
    {
        GLuint sha = glCreateShader(i == 0 ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
        glShaderSource(sha, 3, string + i*3, NULL);
        glCompileShader(sha);
        int success;
        glGetShaderiv(sha, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            int length;
            glGetShaderiv(sha, GL_INFO_LOG_LENGTH, &length);
            char message[length];
            glGetShaderInfoLog(sha, length, &length, message);
            glDeleteShader(sha);
            fprintf(stderr, "ERROR: fail to compile %s shader. file %s\n%s\n",
                    i == 0 ? "vertex" : "fragment", filename, message);
            return 1;
        }
        newShader[i] = sha;
    }

    GLsizei NbShaders;
    GLuint oldShader[2];
    glGetAttachedShaders(prog, 2, &NbShaders, oldShader);
    for (int i=0; i<NbShaders; i++)
    {
        glDetachShader(prog, oldShader[i]);
        glDeleteShader(oldShader[i]);
    }
    glAttachShader(prog, newShader[0]);
    glAttachShader(prog, newShader[1]);
    glLinkProgram(prog);
    glValidateProgram(prog);
    return 0;
}

#include <time.h>

static bool loadShaderD(int N, long *lastModTime, GLuint prog, const char *filename)
{
    const time_t stop = clock();
    struct stat libStat;
    int err = stat(filename, &libStat);
    if (err == 0 && *lastModTime != libStat.st_mtime)
    {
        err = loadShaderS(N, prog, filename);
        if (err >= 0)
        {
            const time_t elapseTime = (clock() - stop) / 1000;
            printf("INFO: loaded file %s. It took %d ms\n", filename, elapseTime);
            *lastModTime = libStat.st_mtime;
            return true;
        }
    }
    return false;
}

bool loadShader1(long *lastModTime, GLuint prog, const char *filename)
{
    return loadShaderD(1, lastModTime, prog, filename);
}

bool loadShader2(long *lastModTime, GLuint prog, const char *filename)
{
    return loadShaderD(2, lastModTime, prog, filename);
}

void *loadPlugin(const char * filename)
{
    static long lastModTime;
    static void *handle = NULL;
    static void *f = NULL;

    const time_t stop = clock();
    struct stat libStat;
    int err = stat(filename, &libStat);
    if (err == 0 && lastModTime != libStat.st_mtime)
    {
        if (handle)
        {
            assert(dlclose(handle) == 0);
        }
        handle = dlopen(filename, RTLD_NOW);
        if (handle)
        {
            const time_t elapseTime = (clock() - stop) / 1000;
            printf("INFO: loaded file %s. It took %d ms\n", filename, elapseTime);
            lastModTime = libStat.st_mtime;
            f = dlsym(handle, "mainAnimation");
            assert(f);
        }
        else
        {
            f = NULL;
        }
    }
    return f;
}

unsigned int loadTexture1(const char *filename)
{
    const time_t stop = clock();
    int w,h,c;
    stbi_uc *data = stbi_load(filename, &w,&h,&c, STBI_grey);
    if (!data)
    {
        fprintf(stderr, "ERROR: file %s not found.\n", filename);
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, w,h);
    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,w,h, GL_RED, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    const time_t elapseTime = (clock() - stop) / 1000;
    printf("INFO: loaded file %s. It took %d ms\n", filename, elapseTime);
    return tex;
}
