#include "glbe.h"
#include <array>
#include <cassert>
#include <QDebug>

static const auto *P1_VS_SRC =
#include "../shaders/glbe_p1_vert.glsl"

static const auto *P1_FS_SRC =
#include "../shaders/glbe_p1_frag.glsl"

static const auto *P2_VS_SRC =
#include "../shaders/glbe_p2_vert.glsl"

static const auto *P2_FS_SRC =
#include "../shaders/glbe_p2_frag.glsl"

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

GLuint createShaderProgram(QOpenGLFunctions *pGL, const char *pVStext, const char *pFStext)
{
    assert(pGL && pVStext && pFStext);

    const auto shdr = pGL->glCreateProgram();

    GLint status;
    char infoLog[1024];

    const auto vs = pGL->glCreateShader(GL_VERTEX_SHADER);
    pGL->glShaderSource(vs, 1, &pVStext, nullptr);
    pGL->glCompileShader(vs);
    pGL->glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        pGL->glGetShaderInfoLog(vs, 1024, nullptr, infoLog);
        qDebug() << infoLog;
        assert(false);
    }

    const auto fs = pGL->glCreateShader(GL_FRAGMENT_SHADER);
    pGL->glShaderSource(fs, 1, &pFStext, nullptr);
    pGL->glCompileShader(fs);
    pGL->glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        pGL->glGetShaderInfoLog(fs, 1024, nullptr, infoLog);
        qDebug() << infoLog;
        assert(false);
    }

    pGL->glAttachShader(shdr, vs);
    pGL->glAttachShader(shdr, fs);
    pGL->glBindAttribLocation(shdr, ATTR_OFFSET, "aOffset");
    pGL->glLinkProgram(shdr);
    pGL->glGetProgramiv(shdr, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        pGL->glGetProgramInfoLog(shdr, 1024, nullptr, infoLog);
        qDebug() << infoLog;
        assert(false);
    }

    pGL->glDeleteShader(vs);
    pGL->glDeleteShader(fs);

    return shdr;
}

void GLRenderingBackend::init(QOpenGLFunctions *glFunctions)
{
    m_gl = glFunctions;
    assert(m_gl != nullptr);
    const static auto reqFeatures = {
        QOpenGLFunctions::Shaders,
        QOpenGLFunctions::Framebuffers
    };

    const auto feats = m_gl->openGLFeatures();
    for (auto f: reqFeatures)
        if ((feats & f) == 0)
            throw Exception { Exception::IllegalOperation, "required OpenGL features are not supported" };

    // Prepare FBO and texture for intermediate rendering
    m_gl->glGenTextures(1, &m_tex);
    m_gl->glGenFramebuffers(1, &m_fbo);
    if (m_tex == 0 || m_fbo == 0)
        throw Exception { Exception::IllegalOperation, "unable to allocate intermediate texture / FBO" };

    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    m_gl->glBindTexture(GL_TEXTURE_2D, m_tex);
    m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FBO_WIDTH, FBO_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex, 0);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_gl->glCullFace(GL_BACK);
    m_gl->glEnable(GL_CULL_FACE);
    m_gl->glEnable(GL_BLEND);
    m_gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_gl->glDisable(GL_DEPTH_TEST);

    m_shdr1st = createShaderProgram(m_gl, P1_VS_SRC, P1_FS_SRC);
    m_shdr2nd = createShaderProgram(m_gl, P2_VS_SRC, P2_FS_SRC);

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

    m_gl->glUseProgram(m_shdr1st);
    m_uPos = m_gl->glGetUniformLocation(m_shdr1st, "uPosition");
    assert(m_uPos > -1);
    m_uSpriteData = m_gl->glGetUniformLocation(m_shdr1st, "uSpriteData");
    assert(m_uSpriteData > -1);

    m_gl->glUseProgram(m_shdr2nd);
    m_uTexture = m_gl->glGetUniformLocation(m_shdr2nd, "uTexture");
    assert(m_uTexture > -1);

    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    m_gl->glEnableVertexAttribArray(ATTR_OFFSET);
    m_gl->glVertexAttribPointer(ATTR_OFFSET, 2, GL_FLOAT, GL_TRUE, 0, 0);

    m_gl->glActiveTexture(GL_TEXTURE0);
}

void GLRenderingBackend::release()
{
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);

    m_gl->glDeleteFramebuffers(1, &m_fbo);
    m_gl->glDeleteTextures(1, &m_tex);
    m_gl->glDeleteBuffers(1, &m_vbo);
    m_gl->glDeleteProgram(m_shdr1st);
    m_gl->glDeleteProgram(m_shdr2nd);
}

void GLRenderingBackend::setBackground(c6502_byte_t color)
{
    assert(m_gl != nullptr);
    const auto c = fromRGB555(m_palette[color & 0x3Fu]);
    m_gl->glClearColor(c[0], c[1], c[2], 1);
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
    m_gl->glGetIntegerv(GL_VIEWPORT, m_viewPort);

    // Render the PPU data to 256x240 texture
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    m_gl->glViewport(0, 0, FBO_WIDTH, FBO_HEIGHT);
    m_gl->glUseProgram(m_shdr1st);
    m_gl->glFrontFace(GL_CCW);

    // Render background
    m_gl->glClear(GL_COLOR_BUFFER_BIT);

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

    // Render FBO contents to screen with scaling
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_gl->glViewport(m_viewPort[0], m_viewPort[1], m_viewPort[2], m_viewPort[3]);
    m_gl->glBindTexture(GL_TEXTURE_2D, m_tex);
    m_gl->glUseProgram(m_shdr2nd);
    m_gl->glUniform1i(m_uTexture, 0);
    m_gl->glFrontFace(GL_CW);

    m_gl->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);
}

