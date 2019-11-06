#include "bus.h"
#include "cpu6502.h"
#include "PPU.h"
#include "Cartridge.h"
#include "log.h"

void Bus::injectCartrige(Cartrige *cart)
{
    m_pCart = cart;

    std::array<c6502_word_t, 4> m = { 0, 0, 0, 0 };
    switch (cart->mirroring())
    {
        case Mirroring::Horizontal:
            m[2] = m[3] = 2u;
            break;
        case Mirroring::Vertical:
            m[1] = m[3] = 1u;
            break;
        case Mirroring::FourScreen:
            for (c6502_word_t i = 1u; i < 4u; i++)
                m[i] = i;
    }
    m_pPPU->setPageMirroring(m);

    m_pCPU->reset();
}

void Bus::generateIRQ()
{
    assert(m_pCPU != nullptr);
    m_pCPU->IRQ();
}

void Bus::generateNMI()
{
    assert(m_pCPU != nullptr);
    m_pCPU->NMI();
}

void Bus::testKeys()
{
}

// Memory request dispatching functions
c6502_byte_t Bus::read(c6502_word_t addr)
{
    switch (addr >> 13)
    {
        case 0:
            return m_ram.Read(addr & 0x7FF);
        case 1:
            // PPU
            assert(m_pPPU != nullptr);
            return m_pPPU->readRegister(addr & 0x0F);
        case 2:
            // APU
            //assert(false && "APU is not yet implemented");
            //break;
            return 0;
        default:
            // Read from the cartridge
            return m_pCart->read(addr);
    }
}

void Bus::write(c6502_word_t addr, c6502_byte_t val)
{
    switch (addr >> 13)
    {
        case 0:
            // To internal RAM
            m_ram.Write(addr & 0x7FF, val);
            break;
        case 1:
            // To PPU registers
            assert(m_pPPU != nullptr);
            return m_pPPU->writeRegister(addr & 0x0F, val);
            break;
        case 2:
            if (addr == 0x4014u)
            {
                // DMA
                assert(m_pPPU != nullptr);
                const c6502_word_t off = static_cast<c6502_word_t>(val) << 8;
                assert(off < 0x800u || off >= 0x6000u);
                auto &dest = m_pPPU->spriteMemory();
                for (c6502_word_t i = 0u; i < 0x100u; i++)
                    dest.Write(i, read(off + i));
            }
            else
            {
                // To APU registers
                //assert(false && "APU is not yet implemented");
            }
            break;
        default:
            // To the cartridge mapper
            m_pCart->write(addr, val);
    }
}
