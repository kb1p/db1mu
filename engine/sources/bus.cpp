#include "bus.h"
#include "cpu6502.h"
#include "PPU.h"
#include "Cartridge.h"
#include "gamepad.h"
#include "log.h"

#include <cassert>
#include <fstream>

void Bus::injectCartrige(Cartrige *cart)
{
    m_pCart = cart;

    // Clear memory
    m_ram.Clear();
    m_vram.Clear();
    m_spriteMem.Clear();

    m_pCPU->reset();

    m_nFrame = 0;
}

void Bus::setCPU(CPU6502 *pCPU) noexcept
{
    assert(pCPU != nullptr);
    m_pCPU = pCPU;
    pCPU->setBus(this);
}

void Bus::setPPU(PPU *pPPU) noexcept
{
    assert(pPPU != nullptr);
    m_pPPU = pPPU;
    pPPU->setBus(this);
}

void Bus::setGamePad(int n, Gamepad *pad) noexcept
{
    assert(n >= 0 && n < 2);
    m_pGamePads[n] = pad;
    pad->setBus(this);
}

static constexpr int PAL_FPS = 50,
                     NTSC_FPS = 60,
                     PAL_LINE_CYCLES = 107,     // 106.56
                     PAL_NMI_LINES = 70,
                     NTSC_LINE_CYCLES = 113,    // 113.33
                     NTSC_NMI_LINES = 20;

void Bus::runFrame()
{
    const int CPL = m_mode == OutputMode::PAL ? PAL_LINE_CYCLES : NTSC_LINE_CYCLES,
              NMI_LINES = m_mode == OutputMode::PAL ? PAL_NMI_LINES : NTSC_NMI_LINES;

    m_nFrame++;

    m_pPPU->startFrame();

    // Visible scanlines
    for (int i = 0; i < 240; i++)
    {
        m_pPPU->drawNextLine();
        m_pCPU->run(CPL);
    }

    m_pPPU->endFrame();

    // Unlock PPU and send NMI signal
    m_pPPU->onBeginVblank();

    if (m_pPPU->isNMIEnabled())
    {
        // Sending of NMI signal from PPU to CPU takes 7 clocks.
        // At this time CPU is still running and VBLANK flag is
        // already set.
        m_pCPU->run(7);
        m_pCPU->NMI();
    }

    // PPU is opened for writinng only during VSYNC
    for (int i = 0; i < NMI_LINES; i++)
        m_pCPU->run(CPL);

    m_pPPU->onEndVblank();
}

int Bus::currentTimeMs() const noexcept
{
    return m_nFrame * 1000 / (m_mode == OutputMode::PAL ? PAL_FPS : NTSC_FPS);
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
            switch (addr)
            {
                case 0x4016u:
                    return m_pGamePads[0] ? m_pGamePads[0]->readRegister() : 0u;
                case 0x4017u:
                    return m_pGamePads[1] ? m_pGamePads[1]->readRegister() : 0u;
                default:
                    // APU
                    //assert(false && "APU is not yet implemented");
                    //break;
                    return 0;
            }
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
            switch (addr)
            {
                case 0x4014u:
                {
                    // DMA
                    const c6502_word_t off = static_cast<c6502_word_t>(val) << 8;
                    assert(off < 0x800u || off >= 0x6000u);
                    for (c6502_word_t i = 0u; i < 0x100u; i++)
                        m_spriteMem.Write(i, readMem(off + i));

                    break;
                }
                case 0x4016u:
                {
                    val &= 1u;
                    if (m_strobeReg == 1u && val == 0u)
                    {
                        if (m_pGamePads[0])
                            m_pGamePads[0]->strobe();
                        if (m_pGamePads[1])
                            m_pGamePads[1]->strobe();
                    }
                    m_strobeReg = val;

                    break;
                }
                //default:
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

static const char MAGIC[] = { 'D', 'B', '1', 'M', 'U', 'S', 'S', 0u, 0u, 0u };

/* Binary state format (to be revised):
 * 0000: MAGIC (10 bytes)
 * 000A: CPU state (7 bytes)
 * 0011: PPU state (27 bytes)
 * 002C: RAM snapshot (2048 bytes)
 * 082C: Sprite memory snapshot (256 bytes)
 * 092C: Video memory shapshot (solid array, 8192 bytes)
 * 292C: Power-independent memory snapshot (8192 bytes)
 */

void Bus::saveState(const char *fileName)
{
    std::ofstream fout { fileName,
                         std::ios_base::out | std::ios_base::binary };


    // Write magic
    fout.write(MAGIC, sizeof(MAGIC));

    // Dump CPU state
    m_pCPU->saveState(fout);

    // Dump PPU state
    m_pPPU->saveState(fout);

    // Dump memory
    m_ram.Save(fout);
    m_spriteMem.Save(fout);
    m_vram.Save(fout);
    m_wram.Save(fout);

    // TODO: add mapper state
}

void Bus::loadState(const char *fileName)
{
    std::ifstream fin { fileName,
                        std::ios_base::in | std::ios_base::binary };

    // Validate magic
    char magicBuf[sizeof(MAGIC)];
    fin.read(magicBuf, sizeof(MAGIC));
    if (memcmp(magicBuf, MAGIC, sizeof(MAGIC)) != 0)
        throw Exception { Exception::IllegalFormat, "wrong magic number" };

    // Read CPU state
    m_pCPU->loadState(fin);

    // Read PPU state
    m_pPPU->loadState(fin);

    // Read memory
    m_ram.Load(fin);
    m_spriteMem.Load(fin);
    m_vram.Load(fin);
    m_wram.Load(fin);

    // TODO: add mapper state
}
