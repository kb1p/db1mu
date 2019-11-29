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

    // NES to RGB
    GLint m_palette[64] = {
        0b010101u, 0b000010u, 0b000010u, 0b010010u,
        0b100001u, 0b100000u, 0b100000u, 0b010000u,
        0b010100u, 0b000100u, 0b000100u, 0b000100u,
        0b000101u, 0b000000u, 0b000000u, 0b000000u,
        0b101010u, 0b000111u, 0b000111u, 0b100011u,
        0b100010u, 0b110001u, 0b110000u, 0b100100u,
        0b100100u, 0b001000u, 0b001000u, 0b001001u,
        0b001010u, 0b000000u, 0b000000u, 0b000000u,
        0b111111u, 0b011011u, 0b011011u, 0b101011u,
        0b110111u, 0b110110u, 0b110101u, 0b111001u,
        0b111001u, 0b101000u, 0b011101u, 0b011110u,
        0b001111u, 0b010101u, 0b000000u, 0b000000u,
        0b111111u, 0b101111u, 0b101011u, 0b101011u,
        0b111011u, 0b111011u, 0b111010u, 0b111110u,
        0b111110u, 0b111110u, 0b101110u, 0b101110u,
        0b101111u, 0b101010u, 0b000000u, 0b000000u
    };

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
