#ifndef BUS_H
#define BUS_H

#include "storage.h"

class CPU6502;
class PPU;
class Cartrige;

enum class OutputMode
{
    PAL, NTSC
};

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

    // Sprite memory, addressed by sprite index (0..63)
    Storage<256> m_spriteMem;

    // Modules
    CPU6502 *m_pCPU = nullptr;
    PPU *m_pPPU = nullptr;
    Cartrige *m_pCart = nullptr;

    const OutputMode m_mode;

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

    // Interrupts dispatching functions
    void generateIRQ();
    void generateNMI();

    void testKeys();

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
