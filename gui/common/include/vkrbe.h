#ifndef VKRBE_H
#define VKRBE_H

#include <PPU.h>
#include <log.h>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <cassert>

class VulkanRenderingBackend: public RenderingBackend
{
    uint8_t m_texData[TEX_WIDTH * TEX_HEIGHT * 4];

public:
    ~VulkanRenderingBackend()
    {
        release();
    }

    void init();
    void resize(int w, int h);
    void release();

    void setLine(const int n,
                 const c6502_byte_t *pColorData,
                 const c6502_byte_t bgColor) override
    {
        setLineToBuf(m_texData, n, pColorData, bgColor);
    }

    void draw() override;
};

#endif
