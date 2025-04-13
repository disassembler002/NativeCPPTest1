#include <jni.h>
#include <string>
#include <unistd.h>
#include <jni.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Texture.h"

#define LOG_TAG "libNative"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace glm;

static const char glVertexShader[] =
        "attribute vec4 vertexPosition;\n"
        "attribute vec2 vertexTextureCord;\n"
        "varying vec2 textureCord;\n"
        "attribute vec3 vertexNormal;\n"
        "varying vec3 fragColour;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 modelView;\n"
        "void main()\n"
        "{\n"
        "    vec3 transformedVertexNormal = normalize((modelView * vec4(vertexNormal, 0.0)).xyz);"
        "    vec3 inverseLightDirection = normalize(vec3(0.0, 1.0, 1.0));\n"
        "    fragColour = vec3(0.0);\n"

        "    vec3 diffuseLightIntensity = vec3(1.0, 1.0, 1.0);\n"
        "    vec3 vertexDiffuseReflectionConstant = vec3(1.0, 1.0, 1.0);\n"
        "    float normalDotLight = max(0.0, dot(transformedVertexNormal, inverseLightDirection));\n"
        "    fragColour += normalDotLight * vertexDiffuseReflectionConstant * diffuseLightIntensity;\n"

        "    vec3 ambientLightIntensity = vec3(0.1, 0.1, 0.1);\n"
        "    vec3 vertexAmbientReflectionConstant = vec3(1.0, 1.0, 1.0);\n"
        "    fragColour += vertexAmbientReflectionConstant * ambientLightIntensity;\n"

        "    vec3 inverseEyeDirection = normalize(vec3(0.0, 0.0, 1.0));\n"
        "    vec3 specularLightIntensity = vec3(1.0, 1.0, 1.0);\n"
        "    vec3 vertexSpecularReflectionConstant = vec3(1.0, 1.0, 1.0);\n"
        "    float shininess = 2.0;\n"
        "    vec3 lightReflectionDirection = reflect(vec3(0) - inverseLightDirection, transformedVertexNormal);\n"
        "    float normalDotReflection = max(0.0, dot(inverseEyeDirection, lightReflectionDirection));\n"
        "    fragColour += pow(normalDotReflection, shininess) * vertexSpecularReflectionConstant * specularLightIntensity;\n"

        "    /* Make sure the fragment colour is between 0 and 1. */"
        "    clamp(fragColour, 0.0, 1.0);\n"

        "    gl_Position = projection * modelView * vertexPosition;\n"
        "    textureCord = vertexTextureCord;\n"
        "}\n";


static const char glFragmentShader[] =
        "precision mediump float;\n"
        "varying vec3 fragColour;\n"
        "uniform sampler2D texture;\n"
        "varying vec2 textureCord;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(fragColour, 1.0) * texture2D(texture, textureCord);\n"
        "}\n";

GLuint loadShader(GLenum shaderType, const char *shaderSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &shaderSource, nullptr);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char *buf = (char *) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, nullptr, buf);
                    LOGE("Could not Compile Shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char *vertexSource, const char *fragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader) {
        return 0;
    }
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader) {
        return 0;
    }
    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char *buf = (char *) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, nullptr, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint lightingProgram;
GLuint vertexLocation;
GLint projectionLocation;
GLint modelViewLocation;
GLuint textureId;
GLuint textureCordLocation;
GLint samplerLocation;
GLuint vertexNormalLocation;

mat4 projectionMatrix(1.0);
mat4 modelViewMatrix(1.0);
float angleDelta = 1;
bool isLeftRotation = true;

bool setupGraphics(int width, int height) {
    lightingProgram = createProgram(glVertexShader, glFragmentShader);
    if (lightingProgram == 0) {
        LOGE ("Could not create program");
        return false;
    }
    if (width <= 0 || height <= 0) {
        LOGE ("Invalid screen dimensions");
        return false;
    }

    vertexLocation = glGetAttribLocation(lightingProgram, "vertexPosition");
    vertexNormalLocation = glGetAttribLocation(lightingProgram, "vertexNormal");
    projectionLocation = glGetUniformLocation(lightingProgram, "projection");
    modelViewLocation = glGetUniformLocation(lightingProgram, "modelView");
    textureCordLocation = glGetAttribLocation(lightingProgram, "vertexTextureCord");
    samplerLocation = glGetUniformLocation(lightingProgram, "texture");

    projectionMatrix = glm::perspective(45.0f, (float) width / (float) height, 0.1f, 100.0f);
    modelViewMatrix = glm::translate(modelViewMatrix, glm::vec3(0.0f, 0.0f, -10.0f));

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);

    textureId = Texture::loadSimpleTexture();
    if (textureId == 0) {
        return false;
    } else {
        return true;
    }
    return true;
}

GLfloat cubeVertices[] = {-1.0f, 1.0f, -1.0f, /* Back. */
                          1.0f, 1.0f, -1.0f,
                          -1.0f, -1.0f, -1.0f,
                          1.0f, -1.0f, -1.0f,
                          -1.0f, 1.0f, 1.0f, /* Front. */
                          1.0f, 1.0f, 1.0f,
                          -1.0f, -1.0f, 1.0f,
                          1.0f, -1.0f, 1.0f,
                          -1.0f, 1.0f, -1.0f, /* Left. */
                          -1.0f, -1.0f, -1.0f,
                          -1.0f, -1.0f, 1.0f,
                          -1.0f, 1.0f, 1.0f,
                          1.0f, 1.0f, -1.0f, /* Right. */
                          1.0f, -1.0f, -1.0f,
                          1.0f, -1.0f, 1.0f,
                          1.0f, 1.0f, 1.0f,
                          -1.0f, -1.0f, -1.0f, /* Top. */
                          -1.0f, -1.0f, 1.0f,
                          1.0f, -1.0f, 1.0f,
                          1.0f, -1.0f, -1.0f,
                          -1.0f, 1.0f, -1.0f, /* Bottom. */
                          -1.0f, 1.0f, 1.0f,
                          1.0f, 1.0f, 1.0f,
                          1.0f, 1.0f, -1.0f
};

GLfloat textureCords[] = {1.0f, 1.0f, /* Back. */
                          0.0f, 1.0f,
                          1.0f, 0.0f,
                          0.0f, 0.0f,
                          0.0f, 1.0f, /* Front. */
                          1.0f, 1.0f,
                          0.0f, 0.0f,
                          1.0f, 0.0f,
                          0.0f, 1.0f, /* Left. */
                          0.0f, 0.0f,
                          1.0f, 0.0f,
                          1.0f, 1.0f,
                          1.0f, 1.0f, /* Right. */
                          1.0f, 0.0f,
                          0.0f, 0.0f,
                          0.0f, 1.0f,
                          0.0f, 1.0f, /* Top. */
                          0.0f, 0.0f,
                          1.0f, 0.0f,
                          1.0f, 1.0f,
                          0.0f, 0.0f, /* Bottom. */
                          0.0f, 1.0f,
                          1.0f, 1.0f,
                          1.0f, 0.0f
};

GLfloat normals[] = {0.0f, 0.0f, -1.0f,            /* Back */
                     0.0f, 0.0f, -1.0f,
                     0.0f, 0.0f, -1.0f,
                     0.0f, 0.0f, -1.0f,
                     0.0f, 0.0f, 1.0f,            /* Front */
                     0.0f, 0.0f, 1.0f,
                     0.0f, 0.0f, 1.0f,
                     0.0f, 0.0f, 1.0f,
                     -1.0f, 0.0, 0.0f,            /* Left */
                     -1.0f, 0.0f, 0.0f,
                     -1.0f, 0.0f, 0.0f,
                     -1.0f, 0.0f, 0.0f,
                     1.0f, 0.0f, 0.0f,             /* Right */
                     1.0f, 0.0f, 0.0f,
                     1.0f, 0.0f, 0.0f,
                     1.0f, 0.0f, 0.0f,
                     0.0f, 1.0f, 0.0f,             /* Top */
                     0.0f, 1.0f, 0.0f,
                     0.0f, 1.0f, 0.0f,
                     0.0f, 1.0f, 0.0f,
                     0.0f, -1.0f, 0.0f,            /* Bottom */
                     0.0f, -1.0f, 0.0f,
                     0.0f, -1.0f, 0.0f,
                     0.0f, -1.0f, 0.0f
};

GLushort indices[] = {0, 2, 3, 0, 1, 3, 4, 6, 7, 4, 5, 7, 8, 9, 10, 11, 8, 10, 12, 13, 14, 15, 12,
                      14, 16, 17, 18, 16, 19, 18, 20, 21, 22, 20, 23, 22};

void renderFrame() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    modelViewMatrix = glm::rotate(modelViewMatrix, glm::radians(angleDelta),
                                  glm::vec3(isLeftRotation ? -1.0f : 1.0f, 0.0f, 0.0f));
    modelViewMatrix = glm::rotate(modelViewMatrix, glm::radians(angleDelta),
                                  glm::vec3(0.0f, isLeftRotation ? -1.0f : 1.0f, 0.0f));

    glUseProgram(lightingProgram);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, cubeVertices);
    glEnableVertexAttribArray(vertexLocation);

    glVertexAttribPointer(textureCordLocation, 2, GL_FLOAT, GL_FALSE, 0, textureCords);
    glEnableVertexAttribArray(textureCordLocation);

    glVertexAttribPointer(vertexNormalLocation, 3, GL_FLOAT, GL_FALSE, 0, normals);
    glEnableVertexAttribArray(vertexNormalLocation);

    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    glUniformMatrix4fv(modelViewLocation, 1, GL_FALSE, &modelViewMatrix[0][0]);

    glUniform1i(samplerLocation, 0);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indices);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_nativecpptest1_NativeLib_init(JNIEnv *env, jclass clazz, jint width, jint height) {
    setupGraphics(width, height);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_nativecpptest1_NativeLib_step(JNIEnv *env, jclass clazz) {
    renderFrame();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_nativecpptest1_NativeLib_swipeRight(JNIEnv *env, jclass clazz) {
    isLeftRotation = false;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_nativecpptest1_NativeLib_swipeLeft(JNIEnv *env, jclass clazz) {
    isLeftRotation = true;
}