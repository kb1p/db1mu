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

    void updateScreen();
    void testKeys();

    // Memory request dispatching functions
    c6502_byte_t read(c6502_word_t addr);
    void write(c6502_word_t addr, c6502_byte_t val);
};

#endif
