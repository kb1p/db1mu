#ifndef PPU_H
#define	PPU_H

#include "storage.h"
#include "bus.h"

/// Interface that must be implemented using a concrete rendering system (e.g. Open GL ES)
class RenderingBackend
{
public:
    RenderingBackend() = delete;
    RenderingBackend(const RenderingBackend&) = delete;
    RenderingBackend(RenderingBackend&&) = delete;

    RenderingBackend &operator=(const RenderingBackend&) = delete;
    RenderingBackend &operator=(RenderingBackend&&) = delete;

    virtual void setBackground(c6502_byte_t color) = 0;
    virtual void setTransparent(c6502_byte_t color) = 0;
    virtual void setSymbol(int x, int y, c6502_byte_t colorData[64]) = 0;
    virtual void draw() = 0;
};

class PPU
{
public:
    explicit PPU(Bus &bus, RenderingBackend *rbe):
        m_bus { bus },
        m_backend { rbe }
    {
    }

    void writeRegister(c6502_word_t n, c6502_byte_t val) noexcept;
    c6502_byte_t readRegister(c6502_word_t n) const noexcept;
    void clock();

private:
    Bus &m_bus;
    Storage<0x4000> m_vram;
    Storage<256> m_spriteMem;
    Storage<0x8> m_registers;
    RenderingBackend *const m_backend;
};

#endif	/* PPU_H */

