#ifndef PPU_H
#define	PPU_H

#include "storage.h"

/// Abstract class that must be implemented using a concrete rendering system (e.g. Open GL ES)
class RenderingBackend
{
protected:
    static constexpr int TEX_WIDTH = 256,
                         TEX_HEIGHT = 240;

    // NES to RGB
    static const uint32_t s_palette[64];

    RenderingBackend() = default;

public:
    RenderingBackend(const RenderingBackend&) = delete;
    RenderingBackend(RenderingBackend&&) = delete;

    RenderingBackend &operator=(const RenderingBackend&) = delete;
    RenderingBackend &operator=(RenderingBackend&&) = delete;

    virtual ~RenderingBackend() = default;

    virtual void setLine(const int n,
                            const c6502_byte_t *pColorData,
                            const c6502_byte_t bgColor) = 0;
    virtual void draw() = 0;
};

class PPU: public Component
{
public:
    PPU() = default;

    // Prohibit copying
    PPU(const PPU&) = delete;
    PPU &operator=(const PPU&) = delete;

    enum Registers: c6502_word_t
    {
        CONTROL1 = 0,
        CONTROL2 = 1,
        STATE = 2,
        SPRMEM_ADDR = 3,
        SPRMEM_DATA = 4,
        SCROLL = 5,
        VIDMEM_ADDR = 6,
        VIDMEM_DATA = 7
    };

    // Externally visible state
    struct State
    {
        bool enableNMI = false,
             bigSprites = false,
             spritesVisible = false,
             backgroundVisible = false,
             allSpritesVisible = false,
             fullBacgroundVisible = false,
             vblank = false,
             sprite0 = false,
             enableWrite = true,
             over8sprites = false;
        c6502_word_t baBkgnd = 0,
                     baSprites = 0,
                     addrIncr = 1,
                     vramAddr = 0,
                     tmpAddr = 0,
                     fineX = 0,
                     sprmemAddr = 0;
        c6502_byte_t vramReadBuf = 0;
        int w = 0;

        c6502_word_t activePage() const noexcept
        {
            const c6502_word_t index = (vramAddr >> 10u) & 0b11u;
            return 0x2000u + index * 0x400u;
        }
    };

    void setBackend(RenderingBackend *rbe) noexcept
    {
        m_pBackend = rbe;
    }

    void writeRegister(c6502_word_t n, c6502_byte_t val) noexcept;
    c6502_byte_t readRegister(c6502_word_t n) noexcept;

    bool isNMIEnabled() const noexcept
    {
        return m_st.enableNMI;
    }

    void onBeginVblank() noexcept;
    void onEndVblank() noexcept;

    // Per-line drawing interface
    void startFrame() noexcept;
    void drawNextLine() noexcept;
    void endFrame() noexcept;

    void reset() noexcept
    {
        m_st = { };
    }

    const State &currentState() const noexcept
    {
        return m_st;
    }

    static constexpr c6502_byte_t TRANSPARENT_PXL = 0x80u;

    size_t saveState(std::ostream &out) override;
    size_t loadState(std::istream &in) override;

private:
    static constexpr int PPR = 256,
                         PPC = 240;

    RenderingBackend *m_pBackend = nullptr;

    State m_st;
    int m_currLine = 0;

    void readCharacterLine(c6502_byte_t *line,
                           const c6502_word_t charInd,
                           const c6502_word_t lineInd,
                           const c6502_word_t baseAddr,
                           const bool fliph,
                           const bool flipv) noexcept;

    void expandColor(c6502_byte_t *p,
                     c6502_byte_t clrHi,
                     const c6502_word_t palAddr) noexcept;
};

#endif	/* PPU_H */

