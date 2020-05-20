#ifndef PPU_H
#define	PPU_H

#include "storage.h"

class PPU: public Component
{
public:
    /// Interface that must be implemented using a concrete rendering system (e.g. Open GL ES)
    class RenderingBackend
    {
        PPU *m_pPPU = nullptr;

    protected:
        RenderingBackend() = default;
        virtual ~RenderingBackend() = default;

    public:
        RenderingBackend(const RenderingBackend&) = delete;
        RenderingBackend(RenderingBackend&&) = delete;

        RenderingBackend &operator=(const RenderingBackend&) = delete;
        RenderingBackend &operator=(RenderingBackend&&) = delete;

        virtual void setLine(const int n, const c6502_byte_t *pColorData) = 0;
        virtual void draw() = 0;

        void setPPUInstance(PPU *pPPU) noexcept
        {
            m_pPPU = pPPU;
        }
    };

    explicit PPU(RenderingBackend *rbe):
        m_pBackend { rbe }
    {
        assert(m_pBackend != nullptr);
        m_pBackend->setPPUInstance(this);
    }

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
                     activePageIndex = 0,
                     vramAddr = 0,
                     sprmemAddr = 0;
        c6502_byte_t scrollV = 0,
                     scrollH = 0,
                     vramReadBuf = 0;

        c6502_word_t activePage() const noexcept
        {
            return 0x2000u + activePageIndex * 0x400u;
        }
    };

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

    const State &currentState() const noexcept
    {
        return m_st;
    }

private:
    struct PageTileInfo
    {
        c6502_word_t pageAddr,
                     charIndex,
                     attrIndex;

        c6502_word_t characterAddress() const noexcept
        {
            return pageAddr + charIndex;
        }

        c6502_word_t attributeAddress() const noexcept
        {
            return pageAddr + attrIndex + 960u;
        }
    };

    static constexpr int PPR = 256,
                         PPC = 240;

    RenderingBackend *const m_pBackend;

    State m_st;
    int m_scrollSwitch = 0;
    int m_currLine = 0;
    c6502_byte_t m_frameVScroll = 0,
                 m_bgColor = 0;

    void readCharacterLine(c6502_byte_t *line,
                           const c6502_word_t charInd,
                           const c6502_word_t lineInd,
                           const c6502_word_t baseAddr,
                           const bool fliph,
                           const bool flipv) noexcept;

    void expandColor(c6502_byte_t *p,
                     c6502_byte_t clrHi,
                     const c6502_word_t palAddr) noexcept;

    PageTileInfo getTile(const int sx, const int sy) noexcept;
};

#endif	/* PPU_H */

