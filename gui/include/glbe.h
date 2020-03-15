#ifndef GL_BE_H
#define GL_BE_H

#include <PPU.h>

#include <QOpenGLFunctions>

class GLRenderingBackend: public PPU::RenderingBackend
{
    static constexpr GLsizei FBO_WIDTH = 256,
                             FBO_HEIGHT = 240;

    QOpenGLFunctions *m_gl = nullptr;
    GLuint m_shdr1st = 0,
           m_shdr2nd = 0,
           m_vbo = 0,
           m_fbo = 0,
           m_tex = 0;
    GLint m_uPos = 0,
          m_uSpriteData = 0,
          m_uTexture = 0;
    GLint m_viewPort[4] = { };

    // NES to RGB
    GLint m_palette[64] = {
        0b011100111001110u,
        0b001000001110001u,
        0b000000000010101u,
        0b010000000010011u,
        0b100010000001110u,
        0b101010000000010u,
        0b101000000000000u,
        0b011110000100000u,
        0b010000010100000u,
        0b000000100000000u,
        0b000000101000000u,
        0b000000011100010u,
        0b000110011101011u,
        0b000000000000000u,
        0b000000000000000u,
        0b000000000000000u,
        0b101111011110111u,
        0b000000111011101u,
        0b001000011111101u,
        0b100000000011110u,
        0b101110000010111u,
        0b111000000001011u,
        0b110110010100000u,
        0b110010100100001u,
        0b100010111000000u,
        0b000001001000000u,
        0b000001010100000u,
        0b000001001000111u,
        0b000001000010001u,
        0b000000000000000u,
        0b000000000000000u,
        0b000000000000000u,
        0b111111111111111u,
        0b001111011111111u,
        0b010111001011111u,
        0b110011000111111u,
        0b111100111111111u,
        0b111110111010110u,
        0b111110111001100u,
        0b111111001100111u,
        0b111101011100111u,
        0b100001101000010u,
        0b010011101101001u,
        0b010111111110011u,
        0b000001110111011u,
        0b011110111101111u,
        0b000000000000000u,
        0b000000000000000u,
        0b111111111111111u,
        0b101011110011111u,
        0b110001101011111u,
        0b110101100111111u,
        0b111111100011111u,
        0b111111100011011u,
        0b111111011110110u,
        0b111111101110101u,
        0b111111110010100u,
        0b111001111110100u,
        0b101011111010111u,
        0b101101111111001u,
        0b100111111111110u,
        0b110001100011000u,
        0b000000000000000u,
        0b000000000000000u
    };

    struct TileData
    {
        int x, y;
        GLint pixels[64];
    };

    static constexpr int MAX_TILES_BEHIND = 128,
                         MAX_TILES_FRONT = 128,
                         MAX_TILES_BACKGROUND = 33 * 31;

    // Character data sorted by layer
    TileData m_layerBehind[MAX_TILES_BEHIND],
             m_layerBg[MAX_TILES_BACKGROUND],
             m_layerFront[MAX_TILES_FRONT];
    int m_nTilesBehind = 0,
        m_nTilesBg = 0,
        m_nTilesFront = 0;

    void renderCharacter(const TileData &chData) const noexcept;

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
