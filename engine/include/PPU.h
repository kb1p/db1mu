#ifndef PPU_H
#define	PPU_H

#include "storage.h"
#include "bus.h"
#include <array>

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

    using SpriteStorage = Storage<256>;

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

    SpriteStorage &spriteMemory() noexcept
    {
        return m_spriteMem;
    }

    // Renders the image and updates the state
    void update() noexcept;

    void setPageMirroring(const std::array<c6502_word_t, 4> &mirroring)
    {
        m_mirroring = mirroring;
    }

private:
    static constexpr c6502_word_t PAL_BG = 0x3F00u,
                                  PAL_SPR = 0x3F10u;

    Bus &m_bus;
    Storage<0x4000> m_vram;
    SpriteStorage m_spriteMem;
    RenderingBackend *const m_pBackend;

    bool m_enableNMI = true,
         m_bigSprites = false,
         m_spritesVisible = true,
         m_backgroundVisible = true,
         m_allSpritesVisible = true,
         m_fullBacgroundVisible = true,
         m_busy = false,
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
    bool m_vramReadError = true;

    // Page mirroring
    std::array<c6502_word_t, 4> m_mirroring = {
        0, 0, 0, 0
    };

    void buildImage() noexcept;

    void readCharacter(c6502_word_t ind,
                       c6502_byte_t (&sym)[64],
                       const c6502_word_t baseAddr,
                       const bool fliph,
                       const bool flipv) noexcept;

    // Read / write to VRAM with respect to page mirroring
    c6502_byte_t readVRAM(c6502_word_t addr) const noexcept
    {
        if (addr >= 0x2000u && addr < 0x3000u)
        {
            const auto pn = (addr - 0x2000u) / 0x400u;
            addr = m_mirroring[pn] * 0x400u + 0x2000u;
        }
        return m_vram.Read(addr);
    }

    void writeVRAM(c6502_word_t addr, c6502_byte_t val) noexcept
    {
        // Palette mirroring
        if (addr >= PAL_BG && addr < PAL_BG + 16 && addr % 4 == 0)
            m_vram.Write(PAL_SPR + (addr - PAL_BG), val);
        else if (addr >= PAL_SPR && addr < PAL_SPR + 16 && addr % 4 == 0)
            m_vram.Write(PAL_BG + (addr - PAL_SPR), val);
        else if (addr >= 0x2000u && addr < 0x3000u)
        {
            // Page mirroring
            const auto pn = (addr - 0x2000u) / 0x400u;
            addr = m_mirroring[pn] * 0x400u + 0x2000u;
        }
        m_vram.Write(addr, val);
    }
};

#endif	/* PPU_H */

