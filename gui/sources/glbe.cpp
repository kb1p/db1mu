#include "glbe.h"
#include "common.h"
#include <cassert>
#include <QDebug>

static const auto *VS_SRC =
#include "../shaders/glbe_vert.glsl"

static const auto *FS_SRC =
#include "../shaders/glbe_frag.glsl"

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
        QOpenGLFunctions::Shaders
    };

    const auto feats = m_gl->openGLFeatures();
    for (auto f: reqFeatures)
        if ((feats & f) == 0)
            throw Exception { Exception::IllegalOperation, "required OpenGL features are not supported" };

    memset(m_texData, 0, TEX_WIDTH * TEX_HEIGHT * 4);

    // Prepare FBO and texture for intermediate rendering
    m_gl->glGenTextures(1, &m_tex);
    if (m_tex == 0)
        throw Exception { Exception::IllegalOperation, "unable to allocate intermediate texture" };

    m_gl->glBindTexture(GL_TEXTURE_2D, m_tex);
    m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);

    m_gl->glCullFace(GL_BACK);
    m_gl->glFrontFace(GL_CCW);
    m_gl->glEnable(GL_CULL_FACE);
    m_gl->glDisable(GL_BLEND);
    m_gl->glDisable(GL_DEPTH_TEST);

    m_shdr = createShaderProgram(m_gl, VS_SRC, FS_SRC);

    m_gl->glGenBuffers(1, &m_vbo);
    assert(m_vbo != GL_NONE);
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    static constexpr GLfloat planeCoords[] = {
        0,  1,
        0,  0,
        1,  0,
        1,  1
    };
    
    m_gl->glBufferData(GL_ARRAY_BUFFER, sizeof(planeCoords), planeCoords, GL_STATIC_DRAW);

    m_gl->glUseProgram(m_shdr);
    m_uTexture = m_gl->glGetUniformLocation(m_shdr, "uTexture");
    assert(m_uTexture > -1);

    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    m_gl->glEnableVertexAttribArray(ATTR_OFFSET);
    m_gl->glVertexAttribPointer(ATTR_OFFSET, 2, GL_FLOAT, GL_TRUE, 0, 0);

    m_gl->glActiveTexture(GL_TEXTURE0);

    m_gl->glClearColor(1, 0, 0, 1);
}

void GLRenderingBackend::release()
{
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);

    m_gl->glDeleteTextures(1, &m_tex);
    m_gl->glDeleteBuffers(1, &m_vbo);
    m_gl->glDeleteProgram(m_shdr);
}

void GLRenderingBackend::setLine(const int n,
                                 const c6502_byte_t *pColorData,
                                 const c6502_byte_t bgColor)
{
    assert(pColorData != nullptr);

    // Convert NES character (NES palette) into tile (RGB palette)
    auto *pDest = m_texData + (TEX_HEIGHT - 1 - n) * TEX_WIDTH * 4;
    for (int i = 0; i < TEX_WIDTH; i++, pDest += 4)
    {
        const auto c = pColorData[i] != PPU::TRANSPARENT ? (pColorData[i] & 0x3Fu) : bgColor;
        assert(c < 64);
        const auto s = m_palette[c];
        constexpr unsigned b5m = 0b11111u;
        pDest[0] = static_cast<uint8_t>(divrnd(((s >> 10) & b5m) * 255, 31));
        pDest[1] = static_cast<uint8_t>(divrnd(((s >> 5) & b5m) * 255, 31));
        pDest[2] = static_cast<uint8_t>(divrnd((s & b5m) * 255, 31));
        pDest[3] = 255u;
    }
}

void GLRenderingBackend::draw()
{
    m_gl->glClear(GL_COLOR_BUFFER_BIT);

    // Upload texture data
    m_gl->glBindTexture(GL_TEXTURE_2D, m_tex);
    m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_texData);

    // Render FBO contents to screen with scaling
    m_gl->glUseProgram(m_shdr);
    m_gl->glUniform1i(m_uTexture, 0);

    m_gl->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);
}

