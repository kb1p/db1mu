#ifndef BUS_H
#define BUS_H

#include "storage.h"

class CPU6502;
class PPU;
class Cartrige;
class Gamepad;

enum class OutputMode
{
    PAL, NTSC
};

// Palettes location in VROM - 0x2000
static constexpr c6502_word_t PAL_BG = 0x3F00u,
                              PAL_SPR = 0x3F10u;

/*!
 * System bus, controls communication between all units, manages main memory.
 * Object of this class must be created prior to everything else.
 */
class Bus
{
    /*** 6502 MEMORY MAP ***/
    // Internal RAM: 0x0000 ~ 0x2000.
    // 0x0000 ~ 0x0100 is a z-page, have special meaning for addressing.
    Storage<0x800> m_ram;

    // Video memory, separate address space
    Storage<0x2000> m_vram;

    // Cartridge permanent RAM
    Storage<0x2000> m_wram;

    // Sprite memory, addressed by sprite index (0..63)
    Storage<256> m_spriteMem;

    // Modules
    CPU6502 *m_pCPU = nullptr;
    PPU *m_pPPU = nullptr;
    Cartrige *m_pCart = nullptr;
    Gamepad *m_pGamePads[2] = { };

    // Gamepad strobing register
    c6502_byte_t m_strobeReg = 0u;

    const OutputMode m_mode;

    int m_nFrame = 0;

public:
    explicit Bus(OutputMode m):
        m_mode { m }
    {
    }

    void setCPU(CPU6502 *pCPU) noexcept
    {
        assert(pCPU != nullptr);
        m_pCPU = pCPU;
    }

    CPU6502 *getCPU() const noexcept
    {
        assert(m_pCPU != nullptr);
        return m_pCPU;
    }

    void setPPU(PPU *pPPU) noexcept
    {
        assert(pPPU != nullptr);
        m_pPPU = pPPU;
    }

    PPU *getPPU() const noexcept
    {
        assert(m_pPPU != nullptr);
        return m_pPPU;
    }

    OutputMode getMode() const noexcept
    {
        return m_mode;
    }

    void injectCartrige(Cartrige *cart);

    Cartrige *getCartrige() const noexcept
    {
        return m_pCart;
    }

    void runFrame();

    int currentFrame() const noexcept
    {
        return m_nFrame;
    }

    int currentTimeMs() const noexcept;

    void setGamePad(int n, Gamepad *pad) noexcept;

    // CPU address space memory requests dispatching functions
    c6502_byte_t readMem(c6502_word_t addr);
    void writeMem(c6502_word_t addr, c6502_byte_t val);

    // PPU address space access functions
    c6502_byte_t readVideoMem(c6502_word_t addr) const noexcept;
    void writeVideoMem(c6502_word_t addr, c6502_byte_t val) noexcept;

    c6502_byte_t readSpriteMem(c6502_word_t addr) const noexcept
    {
        return m_spriteMem.Read(addr);
    }

    void writeSpriteMem(c6502_word_t addr, c6502_byte_t val) noexcept
    {
        m_spriteMem.Write(addr, val);
    }
};

#endif
