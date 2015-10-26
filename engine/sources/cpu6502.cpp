/*
 * 6502 CPU emulation routines.
 *
 * This file contains only basic CPU routines; opcode implementations
 * are located in a separate module "opcodes.cpp".
 *
 * TODO: replace all these magic numbers with #defines at least.
 */

#include "cpu6502.h"
#include "opcodes.h"
#include "Cartridge.h"
#include "PPU.h"
#include <stddef.h>

// Tacts per period: { PAL, NTSC }
static const int TPP[2] = { 59182, 71595 };

// Num. cycles per 6502 instruction
static const c6502_byte_t CYCLES[256] =
{
    /*0x00*/ 7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
    /*0x10*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x20*/ 6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
    /*0x30*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x40*/ 6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
    /*0x50*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x60*/ 6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
    /*0x70*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x80*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /*0x90*/ 2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
    /*0xA0*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /*0xB0*/ 2,5,2,5,4,4,4,4,2,4,2,4,4,4,4,4,
    /*0xC0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /*0xD0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0xE0*/ 2,6,3,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /*0xF0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
};

CPU6502::OpHandler CPU6502::s_ophandlers[OPCODE_COUNT];

/*** CPU class implementation ***/
CPU6502::CPU6502(Mode mode)
    : m_mode(mode)
    , m_state(STATE_HALTED)
    , m_period(0)
    , m_activeCartrige(0)
{
    // Static initializer
    static bool staticInitComplete = false;

    if (!staticInitComplete)
    {
        for (int i = 0; i < OPCODE_COUNT; i++)
            s_ophandlers[i] = nullptr;

        // handler_array[opcode] = &opcode_handler_function
        #define OPASGN(code) s_ophandlers[code] = &CPU6502::op_##code;

        // assign handlers for all opcodes
        FOR_EACH_OPCODE(OPASGN)

        staticInitComplete = true;
    }
}


c6502_byte_t CPU6502::readMem(c6502_word_t addr)
{
    switch (addr >> 13)
    {
        case 0:
            return m_ram.Read(addr & 0x7FF);
        case 1:
        case 2:
            return readIO(addr);
        case 3:
            return m_activeCartrige->wram.Read(addr & 0x1FFF);
        case 4:
        case 5:
            return m_activeCartrige->rom[0].Read(addr & 0x1FFF);
        case 6:
        case 7:
            return m_activeCartrige->rom[1].Read(addr & 0x1FFF);
    }
}

void CPU6502::writeMem(c6502_word_t addr, c6502_byte_t val)
{
    switch (addr >> 13)
    {
        case 0:
            // To internal RAM
            m_ram.Write(addr & 0x7FF, val);
            break;
        case 1:
            m_ppu->GetRegisters().Write(addr & 0x1F, val);
        case 2:
            // To GPU or APU registers
            writeIO(addr, val);
            break;
        case 3:
            // To cartridge RAM
            m_activeCartrige->wram.Write(addr & 0x1FFF, val);
            break;
        default:
            // Write to registers of memory controller on cartridge
            writeMMC(addr, val);
    }
}

void CPU6502::updateScreen()
{
}

void CPU6502::testKeys()
{
}

void CPU6502::reset()
{
    m_regs.a = m_regs.x = m_regs.y = 0;
    m_regs.p = 0x22;
    m_regs.s = 0xFF;
    m_regs.pc.B.l = readMem(0xFFFC);
    m_regs.pc.B.h = readMem(0xFFFD);
    m_period = TPP[m_mode];
    m_state = STATE_RUN;
}

// Handle maskable interrupt
void CPU6502::IRQ()
{

}

// Handle non-maskable interrupt
void CPU6502::NMI()
{
    push(m_regs.pc.B.h);
    push(m_regs.pc.B.l);
    push(m_regs.p);

    m_regs.pc.B.l = readMem(0xFFFA);
    m_regs.pc.B.h = readMem(0xFFFB);

    m_period -= 7;
}

void CPU6502::InjectCartrige(Cartrige* cartridge)
{
    m_activeCartrige = cartridge;
    reset();
}


void CPU6502::Clock()
{
    if (m_state == STATE_RUN)
    {
        m_period -= step();
        if (m_period <= 0)
        {
            m_period += TPP[m_mode];
            updateScreen();
            testKeys();
            NMI();
        }
    }
    else
    {
        // TODO: log
    }
}

c6502_byte_t CPU6502::step()
{
    c6502_byte_t opcode = advance();

    OpHandler oph = s_ophandlers[opcode];
    if (oph)
    {
        (this->*oph)();
        return CYCLES[opcode];
    }
    else
    {
        m_state = STATE_ERROR;

        // TODO: add error handling
        return 0;
    }
}
