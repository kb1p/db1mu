#ifndef GL_BE_H
#define GL_BE_H

#include <PPU.h>
#include <log.h>
#include <cstdint>
#include <cassert>

template <typename IGL>
class GLRenderingBackend: public RenderingBackend
{
    enum Attributes: GLuint
    {
        ATTR_OFFSET = 0
    };

    IGL *m_gl = nullptr;
    GLuint m_shdr = 0,
           m_vbo = 0,
           m_tex = 0;
    GLint m_uPos = 0,
          m_uSpriteData = 0,
          m_uTexture = 0;
    int m_vpWidth = 0,
        m_vpHeight = 0;

    uint8_t m_texData[TEX_WIDTH * TEX_HEIGHT * 4];

public:
    ~GLRenderingBackend()
    {
        release();
    }

    void init(IGL *glFunctions);
    void resize(int w, int h);
    void release();

    void setLine(const int n,
                 const c6502_byte_t *pColorData,
                 const c6502_byte_t bgColor) override;
    void draw() override;
};

template <typename IGL>
void GLRenderingBackend<IGL>::init(IGL *glFunctions)
{
    m_gl = glFunctions;
    assert(m_gl != nullptr);

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

    static const auto *VS_SRC =
    #include "../shaders/glbe_vert.glsl"

    static const auto *FS_SRC =
    #include "../shaders/glbe_frag.glsl"

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
        Log::e("Vertex shader compilation failure: %s", infoLog);
        assert(false);
    }

    const auto fs = m_gl->glCreateShader(GL_FRAGMENT_SHADER);
    m_gl->glShaderSource(fs, 1, &FS_SRC, nullptr);
    m_gl->glCompileShader(fs);
    m_gl->glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        m_gl->glGetShaderInfoLog(fs, 1024, nullptr, infoLog);
        Log::e("Fragment shader compilation failure: %s", infoLog);
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
        Log::e("Shader program linking failure: %s", infoLog);
        assert(false);
    }

    m_gl->glDeleteShader(vs);
    m_gl->glDeleteShader(fs);

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
}

template <typename IGL>
void GLRenderingBackend<IGL>::resize(int w, int h)
{
    m_vpWidth = w;
    m_vpHeight = h;
}

template <typename IGL>
void GLRenderingBackend<IGL>::release()
{
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);

    m_gl->glDeleteTextures(1, &m_tex);
    m_gl->glDeleteBuffers(1, &m_vbo);
    m_gl->glDeleteProgram(m_shdr);
}

template <typename IGL>
void GLRenderingBackend<IGL>::setLine(const int n,
                                      const c6502_byte_t *pColorData,
                                      const c6502_byte_t bgColor)
{
    assert(pColorData != nullptr);

    // Convert NES character (NES palette) into tile (RGB palette)
    auto *pDest = m_texData + (TEX_HEIGHT - 1 - n) * TEX_WIDTH * 4;
    for (int i = 0; i < TEX_WIDTH; i++, pDest += 4)
    {
        const auto c = pColorData[i] != PPU::TRANSPARENT_PXL ? (pColorData[i] & 0x3Fu) : bgColor;
        assert(c < 64);
        const auto s = s_palette[c];
        constexpr unsigned b5m = 0b11111u;
        pDest[0] = static_cast<uint8_t>(divrnd(((s >> 10) & b5m) * 255, 31));
        pDest[1] = static_cast<uint8_t>(divrnd(((s >> 5) & b5m) * 255, 31));
        pDest[2] = static_cast<uint8_t>(divrnd((s & b5m) * 255, 31));
        pDest[3] = 255u;
    }
}

template <typename IGL>
void GLRenderingBackend<IGL>::draw()
{
    m_gl->glClearColor(1, 0, 0, 1);
    m_gl->glClear(GL_COLOR_BUFFER_BIT);

    // Upload texture data
    m_gl->glActiveTexture(GL_TEXTURE0);
    m_gl->glBindTexture(GL_TEXTURE_2D, m_tex);
    m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_texData);

    // Render FBO contents to screen with scaling
    m_gl->glViewport(0, 0, m_vpWidth, m_vpHeight);
    m_gl->glUseProgram(m_shdr);
    m_gl->glUniform1i(m_uTexture, 0);

    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    m_gl->glEnableVertexAttribArray(ATTR_OFFSET);
    m_gl->glVertexAttribPointer(ATTR_OFFSET, 2, GL_FLOAT, GL_TRUE, 0, nullptr);
    m_gl->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);
}

#endif
