#ifndef GL_BE_H
#define GL_BE_H

#include <PPU.h>

#include <QOpenGLFunctions>

class GLRenderingBackend: public PPU::RenderingBackend
{
    QOpenGLFunctions *m_gl = nullptr;
    GLuint m_shdr = 0,
           m_vbo = 0;
    GLint m_uPos = 0,
          m_uSpriteData = 0;

public:
    ~GLRenderingBackend()
    {
        release();
    }

    void init(QOpenGLFunctions *glFunctions);
    void release();

    void setBackground(c6502_byte_t color) override;
    void setSymbol(Layer l, int x, int y, c6502_byte_t colorData[64]) override;
    void draw() override;
};

#endif
