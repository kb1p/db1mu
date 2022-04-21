#ifndef BUS_H
#define BUS_H

#include "storage.h"

class CPU6502;
class PPU;
class APU;
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

    // Video memory: nametables
    Storage<0x1000> m_vramNS;

    // Video memory: palettes
    Storage<0x20> m_vramPal;

    // Sprite memory, addressed by sprite index (0..63)
    Storage<256> m_spriteMem;

    // Modules
    CPU6502 *m_pCPU = nullptr;
    PPU *m_pPPU = nullptr;
    APU *m_pAPU = nullptr;
    Cartrige *m_pCart = nullptr;
    Gamepad *m_pGamePads[2] = { };

    // Gamepad strobing register
    c6502_byte_t m_strobeReg = 0u;

    OutputMode m_mode;

    int m_nFrame = 0;
    float m_remClk = 0.0f;

public:
    Bus(OutputMode m):
        m_mode { m }
    {
    }

    Bus(const Bus&) = delete;
    Bus &operator=(const Bus&) = delete;

    void setCPU(CPU6502 *pCPU) noexcept;

    CPU6502 *getCPU() const noexcept
    {
        assert(m_pCPU != nullptr);
        return m_pCPU;
    }

    void setPPU(PPU *pPPU) noexcept;

    PPU *getPPU() const noexcept
    {
        assert(m_pPPU != nullptr);
        return m_pPPU;
    }

    void setAPU(APU *pAPU) noexcept;

    APU *getAPU() const noexcept
    {
        return m_pAPU;
    }

    OutputMode getMode() const noexcept
    {
        return m_mode;
    }

    void reset(OutputMode mode);

    void reset()
    {
        reset(m_mode);
    }

    void injectCartrige(Cartrige *cart);

    Cartrige *getCartrige() const noexcept
    {
        return m_pCart;
    }

    void triggerNMI() noexcept;

    void runFrame();

    int currentFrame() const noexcept
    {
        return m_nFrame;
    }

    int currentTimeMs() const noexcept;

    void setGamePad(int n, Gamepad *pad) noexcept;

    // CPU address space memory requests dispatching functions
    c6502_byte_t readMem(c6502_word_t addr) noexcept;
    void writeMem(c6502_word_t addr, c6502_byte_t val) noexcept;

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

    void saveState(const char *fileName);
    void loadState(const char *fileName);

    int clocksPerFrame() const noexcept;
};

#endif
