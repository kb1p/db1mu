#ifndef GL_BE_H
#define GL_BE_H

#include "../../engine/include/PPU.h"

#include <QOpenGLFunctions>

class GLRenderingBackend: public RenderingBackend
{
    QOpenGLFunctions *const m_gl;
    GLuint m_spriteAtlas;

public:
    explicit GLRenderingBackend(QOpenGLFunctions *glFunctions);

};

#endif
