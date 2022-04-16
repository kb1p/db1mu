#include "bus.h"
#include "cpu6502.h"
#include "PPU.h"
#include "APU.h"
#include "Cartridge.h"
#include "gamepad.h"
#include "log.h"

#include <cassert>
#include <fstream>
#include <cmath>

void Bus::reset(OutputMode mode)
{
    m_mode = mode;

    // Clear memory
    m_ram.Clear();
    m_vramNS.Clear();
    m_vramPal.Clear();
    m_spriteMem.Clear();

    // Send reset commands to PPU and CPU
    m_pPPU->reset();
    m_pCPU->reset();
    m_pAPU->reset();

    m_nFrame = 0;
}

void Bus::injectCartrige(Cartrige *cart)
{
    m_pCart = cart;

    reset();
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

void Bus::setAPU(APU *pAPU) noexcept
{
    assert(pAPU != nullptr);
    m_pAPU = pAPU;
    pAPU->setBus(this);
}

void Bus::setGamePad(int n, Gamepad *pad) noexcept
{
    assert(n >= 0 && n < 2);
    m_pGamePads[n] = pad;
    pad->setBus(this);
}

void Bus::triggerNMI() noexcept
{
    // Sending of NMI signal from PPU to CPU takes 7 clocks.
    // At this time CPU is still running and VBLANK flag is
    // already set.
    m_pCPU->run(7);
    m_pCPU->NMI();
}

static constexpr int PAL_FPS = 50,
                     NTSC_FPS = 60,
                     PAL_NMI_LINES = 70,
                     NTSC_NMI_LINES = 20;

static constexpr float PAL_LINE_CYCLES = 106.56f,
                       NTSC_LINE_CYCLES = 113.33f;


int Bus::clocksPerFrame() const noexcept
{
    const auto CPL = m_mode == OutputMode::PAL ? PAL_LINE_CYCLES : NTSC_LINE_CYCLES;
    const auto NMI_LINES = m_mode == OutputMode::PAL ? PAL_NMI_LINES : NTSC_NMI_LINES;
    return std::lround((240 + NMI_LINES) * CPL);
}

void Bus::runFrame()
{
    const int CPL = std::lround(m_mode == OutputMode::PAL ? PAL_LINE_CYCLES : NTSC_LINE_CYCLES),
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
        triggerNMI();

    // PPU is opened for writinng only during VSYNC
    for (int i = 0; i < NMI_LINES; i++)
        m_pCPU->run(CPL);

    m_pPPU->onEndVblank();

    // Clock APU
    m_pAPU->runFrame();
}

int Bus::currentTimeMs() const noexcept
{
    return m_nFrame * 1000 / (m_mode == OutputMode::PAL ? PAL_FPS : NTSC_FPS);
}

// Memory request dispatching functions
c6502_byte_t Bus::readMem(c6502_word_t addr) noexcept
{
    c6502_byte_t rv = 0;
    switch (addr >> 13)
    {
        case 0:
            rv = m_ram.Read(addr & 0x7FFu);
            break;
        case 1:
            // PPU
            assert(m_pPPU != nullptr);
            rv = m_pPPU->readRegister(addr & 0x0Fu);
            break;
        case 2:
            switch (addr)
            {
                case 0x4016u:
                    rv = m_pGamePads[0] ? m_pGamePads[0]->readRegister() : 0u;
                    break;
                case 0x4017u:
                    rv = m_pGamePads[1] ? m_pGamePads[1]->readRegister() : 0u;
                    break;
                default:
                    assert(m_pAPU != nullptr);
                    rv = m_pAPU->readRegister(addr & 0x1Fu);
                    break;
            }
            break;
        case 3:
            rv = m_wram.Read(addr & 0x1FFFu);
            break;
        default:
            // Read from the cartridge
            try
            {
                rv = m_pCart->mapper()->readROM(addr);
            }
            catch (const Exception &ex)
            {
                Log::e("[bus] Failed to read cart's ROM: %s", ex.message());
            }
    }

    return rv;
}

void Bus::writeMem(c6502_word_t addr, c6502_byte_t val) noexcept
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
                default:
                    assert(m_pAPU != nullptr);
                    m_pAPU->writeRegister(addr & 0x1Fu, val);
            }
            break;
        case 3:
            m_wram.Write(addr & 0x1FFFu, val);
            break;
        default:
            // To the cartridge mapper
            try
            {
                m_pCart->mapper()->writeRAM(addr, val);
            }
            catch (const Exception &ex)
            {
                Log::e("[bus] Failed to write to cart's memory: %s", ex.message());
            }
    }
}

c6502_byte_t Bus::readVideoMem(c6502_word_t addr) const noexcept
{
    c6502_byte_t v = 0u;
    if (addr >= 0x3F00u)
        v = m_vramPal.Read(addr & 0x1Fu);
    else if (addr >= 0x2000u)
        v = m_vramNS.Read(addr & 0xFFFu);
    else
    {
        try
        {
            if (m_pCart->mapper()->hasRAM())
                v = m_pCart->mapper()->readRAM(addr);
            else
                v = m_pCart->mapper()->readVROM(addr);
        }
        catch (const Exception &ex)
        {
            Log::e("[bus] Failed to read cart's RAM / VROM: %s", ex.message());
        }
    }

    return v;
}

void Bus::writeVideoMem(c6502_word_t addr, c6502_byte_t val) noexcept
{
    const auto mt = m_pCart->mirroring();

    if (addr >= 0x3F00u)
    {
        addr &= 0x1Fu;
        m_vramPal.Write(addr, val);
        if ((addr & 0x3u) == 0u)
            m_vramPal.Write(addr ^ 0x10u, val);
    }
    else if (addr >= 0x2000u)
    {
        addr &= 0xFFFu;
        m_vramNS.Write(addr, val);
        switch (mt)
        {
            case Mirroring::Horizontal:
                m_vramNS.Write(addr ^ 0x400u, val);
                break;
            case Mirroring::Vertical:
                m_vramNS.Write(addr ^ 0x800u, val);
            default:
                break;
        }
    }
    else
    {
        assert(m_pCart->mapper()->hasRAM());
        try
        {
            m_pCart->mapper()->writeRAM(addr, val);
        }
        catch (const Exception &ex)
        {
            Log::e("[bus] Failed to write to cart's RAM: %s", ex.message());
        }
    }
}

static const char MAGIC[] = { 'D', 'B', '1', 'M', 'U', 'S', 'S', 'v', '1', 0u };

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
    m_vramNS.Save(fout);
    m_vramPal.Save(fout);
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
    m_vramNS.Load(fin);
    m_vramPal.Load(fin);
    m_wram.Load(fin);

    // TODO: add mapper state
}
