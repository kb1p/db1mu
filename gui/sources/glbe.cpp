#include "glbe.h"
#include <array>
#include <cassert>
#include <QDebug>

static const auto *VS_SRC =
#include "../shaders/glbe_vert.glsl"

static const auto *FS_SRC =
#include "../shaders/glbe_frag.glsl"

std::array<GLfloat, 3> fromRGB555(unsigned c) noexcept
{
    constexpr GLfloat stp = 1.0f / 31.0f;
    std::array<GLfloat, 3> r;

    constexpr unsigned b5m = 0b11111u;
    r[2] = stp * static_cast<float>(c & b5m);
    r[1] = stp * static_cast<float>((c >> 5) & b5m);
    r[0] = stp * static_cast<float>((c >> 10) & b5m);

    return r;
}

enum Attributes: GLuint
{
    ATTR_OFFSET = 0
};

void GLRenderingBackend::init(QOpenGLFunctions *glFunctions)
{
    m_gl = glFunctions;
    assert(m_gl != nullptr);
    const static auto reqFeatures = {
        QOpenGLFunctions::Shaders
    };

    const auto feats = m_gl->openGLFeatures();
    for (auto f: reqFeatures)
        if ((feats & f) == 0)
            throw Exception { Exception::IllegalOperation, "required OpenGL features are not supported" };

    m_gl->glFrontFace(GL_CCW);
    m_gl->glCullFace(GL_BACK);
    m_gl->glEnable(GL_CULL_FACE);
    m_gl->glEnable(GL_BLEND);
    m_gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_gl->glDisable(GL_DEPTH_TEST);

    m_shdr = m_gl->glCreateProgram();

    GLint status;
    char infoLog[1024];

    const auto vs = m_gl->glCreateShader(GL_VERTEX_SHADER);
    m_gl->glShaderSource(vs, 1, &VS_SRC, nullptr);
    m_gl->glCompileShader(vs);
    m_gl->glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        m_gl->glGetShaderInfoLog(vs, 1024, nullptr, infoLog);
        qDebug() << infoLog;
        assert(false);
    }

    const auto fs = m_gl->glCreateShader(GL_FRAGMENT_SHADER);
    m_gl->glShaderSource(fs, 1, &FS_SRC, nullptr);
    m_gl->glCompileShader(fs);
    m_gl->glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        m_gl->glGetShaderInfoLog(fs, 1024, nullptr, infoLog);
        qDebug() << infoLog;
        assert(false);
    }

    m_gl->glAttachShader(m_shdr, vs);
    m_gl->glAttachShader(m_shdr, fs);
    m_gl->glBindAttribLocation(m_shdr, ATTR_OFFSET, "aOffset");
    m_gl->glLinkProgram(m_shdr);
    m_gl->glGetProgramiv(m_shdr, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        m_gl->glGetProgramInfoLog(m_shdr, 1024, nullptr, infoLog);
        qDebug() << infoLog;
        assert(false);
    }

    m_gl->glDeleteShader(vs);
    m_gl->glDeleteShader(fs);

    m_gl->glGenBuffers(1, &m_vbo);
    assert(m_vbo != GL_NONE);
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    static constexpr GLfloat planeCoords[] = {
        0,  0,
        0,  1,
        1,  1,
        1,  0
    };
    
    m_gl->glBufferData(GL_ARRAY_BUFFER, sizeof(planeCoords), planeCoords, GL_STATIC_DRAW);

    m_gl->glUseProgram(m_shdr);

    m_uPos = m_gl->glGetUniformLocation(m_shdr, "uPosition");
    assert(m_uPos > -1);
    m_uSpriteData = m_gl->glGetUniformLocation(m_shdr, "uSpriteData");
    assert(m_uSpriteData > -1);

    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    m_gl->glEnableVertexAttribArray(ATTR_OFFSET);
    m_gl->glVertexAttribPointer(ATTR_OFFSET, 2, GL_FLOAT, GL_TRUE, 0, 0);
}

void GLRenderingBackend::release()
{
}

void GLRenderingBackend::setBackground(c6502_byte_t color)
{
    assert(m_gl != nullptr);
    const auto c = fromRGB555(m_palette[color & 0x3Fu]);
    m_gl->glClearColor(c[0], c[1], c[2], 1);
    m_gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLRenderingBackend::setSymbol(Layer l, int x, int y, c6502_byte_t colorData[64])
{
    TileData *pChar = nullptr;
    switch (l)
    {
        case Layer::BEHIND:
            pChar = &m_layerBehind[m_nTilesBehind++];
            Q_ASSERT(m_nTilesBehind <= MAX_TILES_BEHIND);
            break;
        case Layer::FRONT:
            pChar = &m_layerFront[m_nTilesFront++];
            Q_ASSERT(m_nTilesFront <= MAX_TILES_FRONT);
            break;
        case Layer::BACKGROUND:
            pChar = &m_layerBg[m_nTilesBg++];
            Q_ASSERT(m_nTilesBg <= MAX_TILES_BACKGROUND);
            break;
    }
    Q_ASSERT(pChar != nullptr);

    pChar->x = x;
    pChar->y = y;

    // Convert NES character (NES palette) into tile (RGB palette)
    for (int i = 0; i < 64; i++)
    {
        const auto &c = colorData[i];
        pChar->pixels[i] = c > 0 ? (0x8000u | m_palette[c & 0x3Fu]) : 0;
    }
}

void GLRenderingBackend::renderCharacter(const TileData &tData) const noexcept
{
    m_gl->glUniform2i(m_uPos, tData.x, tData.y);
    m_gl->glUniform1iv(m_uSpriteData, 64, tData.pixels);

    m_gl->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void GLRenderingBackend::draw()
{
    // Render characters in proper order
    int i;
    for (i = 0; i < m_nTilesBehind; i++)
        renderCharacter(m_layerBehind[i]);
    for (i = 0; i < m_nTilesBg; i++)
        renderCharacter(m_layerBg[i]);
    for (i = 0; i < m_nTilesFront; i++)
        renderCharacter(m_layerFront[i]);

    // Reset lists
    m_nTilesBehind = m_nTilesBg = m_nTilesFront = 0;
}

