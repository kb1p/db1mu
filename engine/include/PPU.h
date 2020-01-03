#ifndef PPU_H
#define	PPU_H

#include "storage.h"
#include "bus.h"

class PPU
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
        enum class Layer
        {
            BEHIND = 0,
            BACKGROUND = 1,
            FRONT = 2
        };

        RenderingBackend(const RenderingBackend&) = delete;
        RenderingBackend(RenderingBackend&&) = delete;

        RenderingBackend &operator=(const RenderingBackend&) = delete;
        RenderingBackend &operator=(RenderingBackend&&) = delete;

        virtual void setBackground(c6502_byte_t color) = 0;
        virtual void setSymbol(Layer l, int x, int y, c6502_byte_t colorData[64]) = 0;
        virtual void draw() = 0;

        void setPPUInstance(PPU *pPPU) noexcept
        {
            m_pPPU = pPPU;
        }
    };

    explicit PPU(Bus &bus, RenderingBackend *rbe):
        m_bus { bus },
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
                     activePage = 0x2000u;
        c6502_word_t vramAddr = 0,
                     sprmemAddr = 0;
        c6502_byte_t scrollV = 0,
                     scrollH = 0;
        bool vramReadError = false;
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
    Bus &m_bus;
    RenderingBackend *const m_pBackend;

    State m_st;
    int m_currScrollReg = 0;
    int m_currLine = 0;

    void readCharacter(c6502_word_t ind,
                       c6502_byte_t (&sym)[64],
                       const c6502_word_t baseAddr,
                       const bool fliph,
                       const bool flipv) noexcept;
};

#endif	/* PPU_H */

