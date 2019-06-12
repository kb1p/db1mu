#ifndef GL_BE_H
#define GL_BE_H

#include "../../engine/include/PPU.h"

#include <QOpenGLFunctions>

class GLRenderingBackend: public RenderingBackend
{
    QOpenGLFunctions *const m_gl;

public:
    explicit GLRenderingBackend(QOpenGLFunctions *glFunctions):
        m_gl { glFunctions }
    {
    }
};

#endif
