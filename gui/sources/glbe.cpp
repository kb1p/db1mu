#include "glbe.h"

GLRenderingBackend::GLRenderingBackend(QOpenGLFunctions *glFunctions):
    m_gl { glFunctions }
{
    const static auto reqFeatures = {
        QOpenGLFunctions::Shaders
    };

    const auto feats = m_gl->openGLFeatures();
    for (auto f: reqFeatures)
        if ((feats & f) == 0)
            throw Exception { Exception::IllegalOperation, "required OpenGL features are not supported" };
}

void GLRenderingBackend::setup(const float *pal)
{
}

