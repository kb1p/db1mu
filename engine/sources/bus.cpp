#include "bus.h"
#include "cpu6502.h"
#include "PPU.h"
#include "Cartridge.h"
#include "log.h"

void Bus::injectCartrige(Cartrige *cart)
{
    m_pCart = cart;

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
c6502_byte_t Bus::readMem(c6502_word_t addr)
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
            return m_pCart->mapper()->readROM(addr);
    }
}

void Bus::writeMem(c6502_word_t addr, c6502_byte_t val)
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
                for (c6502_word_t i = 0u; i < 0x100u; i++)
                    m_spriteMem.Write(i, readMem(off + i));
            }
            else
            {
                // To APU registers
                //assert(false && "APU is not yet implemented");
            }
            break;
        default:
            // To the cartridge mapper
            m_pCart->mapper()->writeRAM(addr, val);
    }
}

c6502_byte_t Bus::readVideoMem(c6502_word_t addr) const noexcept
{
    if (addr < 0x2000u)
        return m_pCart->mapper()->readVROM(addr);
    else
        return m_vram.Read(addr - 0x2000u);
}

void Bus::writeVideoMem(c6502_word_t addr, c6502_byte_t val) noexcept
{
    assert(addr >= 0x2000u);
    addr -= 0x2000u;

    const auto mt = m_pCart->mirroring();

    // Palette mirroring
    if (addr >= PAL_BG && addr < PAL_BG + 16 && addr % 4 == 0)
        m_vram.Write(PAL_SPR + (addr - PAL_BG), val);
    else if (addr >= PAL_SPR && addr < PAL_SPR + 16 && addr % 4 == 0)
        m_vram.Write(PAL_BG + (addr - PAL_SPR), val);
    else if (addr < 0x3000u && mt != Mirroring::FourScreen)
    {
        // Page mirroring
        constexpr c6502_word_t MAP_V[] = { 2, 3, 0, 1 },
                               MAP_H[] = { 1, 0, 3, 2 };

        auto pn = addr / 0x400u;
        switch (mt)
        {
            case Mirroring::Horizontal:
                pn = MAP_H[pn];
                break;
            case Mirroring::Vertical:
                pn = MAP_V[pn];
                break;
            default:
                assert(false && "unexpected mirroring type");
        }
        m_vram.Write(pn * 0x400u + addr % 0x400u, val);
    }
    m_vram.Write(addr, val);
}
