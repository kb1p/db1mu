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

    void writeRegister(c6502_word_t n, c6502_byte_t val) noexcept;
    c6502_byte_t readRegister(c6502_word_t n) noexcept;

    // Renders the image and updates the state
    void update() noexcept;

private:
    Bus &m_bus;
    RenderingBackend *const m_pBackend;

    bool m_enableNMI = true,
         m_bigSprites = false,
         m_spritesVisible = true,
         m_backgroundVisible = true,
         m_allSpritesVisible = true,
         m_fullBacgroundVisible = true,
         m_vblank = true,
         m_sprite0 = false,
         m_enableWrite = true;
    c6502_word_t m_baBkgnd = 0,
                 m_baSprites = 0,
                 m_addrIncr = 1,
                 m_activePage = 0x2000u;
    c6502_word_t m_vramAddr = 0,
                 m_sprmemAddr = 0;
    int m_spritesOnLine = 0;
    c6502_byte_t m_scrollV = 0,
                 m_scrollH = 0;
    int m_currScrollReg = 0;
    bool m_vramReadError = false;

    void buildImage() noexcept;

    void readCharacter(c6502_word_t ind,
                       c6502_byte_t (&sym)[64],
                       const c6502_word_t baseAddr,
                       const bool fliph,
                       const bool flipv) noexcept;
};

#endif	/* PPU_H */

