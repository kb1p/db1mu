#ifndef GLFUNCWRP_H
#define GLFUNCWRP_H

#include <SDL2/SDL_opengles2.h>
#include <utility>

struct GLFunctionsWrapper
{
#define WRAP_VOID(name) \
    template <typename... Args> \
    void name(Args&&... as) { ::name(std::forward<Args>(as)...); }
#define WRAP_RET(name, rt) \
    template <typename... Args> \
    rt name(Args&&... as) { return ::name(std::forward<Args>(as)...); }

    WRAP_VOID(glGenTextures) 
    WRAP_VOID(glBindTexture)
    WRAP_VOID(glTexImage2D)
    WRAP_VOID(glTexParameteri)
    WRAP_VOID(glCullFace)
    WRAP_VOID(glFrontFace)
    WRAP_VOID(glEnable)
    WRAP_VOID(glDisable)
    WRAP_RET(glCreateProgram, GLuint)
    WRAP_RET(glCreateShader, GLuint)
    WRAP_VOID(glShaderSource)
    WRAP_VOID(glCompileShader)
    WRAP_VOID(glGetShaderiv)
    WRAP_VOID(glGetShaderInfoLog)
    WRAP_VOID(glGetProgramInfoLog)
    WRAP_VOID(glAttachShader)
    WRAP_VOID(glBindAttribLocation)
    WRAP_VOID(glLinkProgram)
    WRAP_VOID(glGetProgramiv)
    WRAP_VOID(glDeleteShader)
    WRAP_VOID(glGenBuffers)
    WRAP_VOID(glBindBuffer)
    WRAP_VOID(glBufferData)
    WRAP_VOID(glUseProgram)
    WRAP_RET(glGetUniformLocation, GLint)
    WRAP_VOID(glEnableVertexAttribArray)
    WRAP_VOID(glVertexAttribPointer)
    WRAP_VOID(glActiveTexture)
    WRAP_VOID(glClearColor)
    WRAP_VOID(glDeleteTextures)
    WRAP_VOID(glDeleteBuffers)
    WRAP_VOID(glDeleteProgram)
    WRAP_VOID(glClear)
    WRAP_VOID(glUniform1i)
    WRAP_VOID(glDrawArrays)
    WRAP_VOID(glViewport)

#undef WRAP_RET
#undef WRAP_FUNC
};

#endif
