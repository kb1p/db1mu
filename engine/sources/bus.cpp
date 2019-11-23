#include "bus.h"
#include "cpu6502.h"
#include "PPU.h"
#include "Cartridge.h"
#include "log.h"

void Bus::injectCartrige(Cartrige *cart)
{
    m_pCart = cart;

    // Clear memory
    m_ram.Clear();
    m_vram.Clear();
    m_spriteMem.Clear();

    m_pCPU->reset();
}

constexpr int divup(int a, int b) noexcept
{
    return a / b + (a % b != 0 ? 1 : 0);
}

void Bus::runFrame()
{
    // TODO: these values need to be tuned
    constexpr int PAL_FREQ = 1773447,
                  NTSC_FREQ = 1789772,
                  PAL_FPS = 50,
                  NTSC_FPS = 60,
                  PAL_FC = divup(PAL_FREQ, PAL_FPS),
                  NTSC_FC = divup(NTSC_FREQ, NTSC_FPS),
                  PAL_LC = divup(PAL_FC, 264),
                  NTSC_LC = divup(NTSC_FC, 264);

    const int clocksPerLine = m_mode == OutputMode::PAL ? PAL_LC : NTSC_LC;
    int clocks = m_mode == OutputMode::PAL ? PAL_FC : NTSC_FC;

    // Most of frame time PPU is busy
    clocks -= m_pCPU->run(clocksPerLine * 240);

    // Render image
    m_pPPU->draw();

    // Unlock PPU and send NMI signal
    m_pPPU->onBeginVblank();

    if (m_pPPU->isNMIEnabled())
        clocks -= m_pCPU->NMI();

    // PPU is opened for writinng only during VSYNC
    clocks -= m_pCPU->run(clocks);

    m_pPPU->onEndVblank();

    assert(clocks <= 0);
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
            return m_ram.Read(addr & 0x7FFu);
        case 1:
            // PPU
            assert(m_pPPU != nullptr);
            return m_pPPU->readRegister(addr & 0x0Fu);
        case 2:
            // APU
            //assert(false && "APU is not yet implemented");
            //break;
            return 0;
        case 3:
            return m_wram.Read(addr & 0x1FFFu);
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
            m_ram.Write(addr & 0x7FFu, val);
            break;
        case 1:
            // To PPU registers
            assert(m_pPPU != nullptr);
            return m_pPPU->writeRegister(addr & 0x0Fu, val);
            break;
        case 2:
            if (addr == 0x4014u)
            {
                // DMA
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
        case 3:
            m_wram.Write(addr & 0x1FFFu, val);
            break;
        default:
            // To the cartridge mapper
            m_pCart->mapper()->writeRAM(addr, val);
    }
}

c6502_byte_t Bus::readVideoMem(c6502_word_t addr) const noexcept
{
    if (addr < 0x2000u)
    {
        if (m_pCart->mapper()->hasRAM())
            return m_pCart->mapper()->readRAM(addr);
        else
            return m_pCart->mapper()->readVROM(addr);
    }
    else
        return m_vram.Read(addr - 0x2000u);
}

void Bus::writeVideoMem(c6502_word_t addr, c6502_byte_t val) noexcept
{
    const auto mt = m_pCart->mirroring();

    if (addr < 0x2000u)
    {
        assert(m_pCart->mapper()->hasRAM());
        m_pCart->mapper()->writeRAM(addr, val);
    }
    else
    {
        constexpr auto PBG = PAL_BG - 0x2000u,
                       PSPR = PAL_SPR - 0x2000u;
        addr -= 0x2000u;
        if (addr < 0x1000u && mt != Mirroring::FourScreen)
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
        // Palette mirroring
        else if (addr >= PBG && addr < PBG + 16 && addr % 4 == 0)
            m_vram.Write(PSPR + (addr - PBG), val);
        else if (addr >= PSPR && addr < PSPR + 16 && addr % 4 == 0)
            m_vram.Write(PBG + (addr - PSPR), val);

        m_vram.Write(addr, val);
    }
}
