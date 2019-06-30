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
#include "debugger.h"
#include "Cartridge.h"
#include "PPU.h"
#include "log.h"
#include <stddef.h>
#include <cassert>

// Tacts per period: { PAL, NTSC }
static const int TPP[2] = { 59182, 71595 };

// Num. cycles per 6502 instruction (not counting the page boundary cross penalty)
// TODO: check these values
static const int CYCLES[256] =
{
    /*0x00*/ 7,6,0,8,3,3,5,5,3,2,2,2,4,4,6,6,
    /*0x10*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x20*/ 6,6,0,8,3,3,5,5,4,2,2,2,4,4,6,6,
    /*0x30*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x40*/ 6,6,0,8,3,3,5,5,3,2,2,2,3,4,6,6,
    /*0x50*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x60*/ 6,6,0,8,3,3,5,5,4,2,2,2,5,4,6,6,
    /*0x70*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x80*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /*0x90*/ 2,6,0,6,4,4,4,4,2,5,2,5,5,5,5,5,
    /*0xA0*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /*0xB0*/ 2,5,0,5,4,4,4,4,2,4,2,4,4,4,4,4,
    /*0xC0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /*0xD0*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0xE0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /*0xF0*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7
};

CPU6502::OpHandler CPU6502::s_ophandlers[OPCODE_COUNT];

/*** CPU class implementation ***/
CPU6502::CPU6502(Mode mode, Bus &bus)
    : m_mode(mode)
    , m_state(STATE_HALTED)
    , m_period(0)
    , m_bus { bus }
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

void CPU6502::updateScreen()
{
}

void CPU6502::testKeys()
{
}

void CPU6502::reset()
{
    m_regs.a = m_regs.x = m_regs.y = 0;
    m_regs.p.reg = 0x22;
    m_regs.s = 0xFF;
    m_regs.pc.B.l = readMem(0xFFFC);
    m_regs.pc.B.h = readMem(0xFFFD);
    m_period = TPP[m_mode];
    m_state = STATE_RUN;
}

// Handle maskable interrupt
void CPU6502::IRQ()
{
    if (m_regs.p.flags.i == 0)
    {
        Log::v("IRQ");

        // Like BRK opcode, but without B flag
        push(m_regs.pc.B.h);
        push(m_regs.pc.B.l);
        push(m_regs.p.reg);
        m_regs.p.flags.i = 1;

        m_regs.pc.B.l = readMem(0xFFFE);
        m_regs.pc.B.h = readMem(0xFFFF);

        m_period -= 7;
    }
}

// Handle non-maskable interrupt
void CPU6502::NMI()
{
    Log::v("NMI");
    push(m_regs.pc.B.h);
    push(m_regs.pc.B.l);
    push(m_regs.p.reg);

    m_regs.pc.B.l = readMem(0xFFFA);
    m_regs.pc.B.h = readMem(0xFFFB);

    m_period -= 7;
}

void CPU6502::clock()
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
        Log::e("Unexpected CPU state (%d)", m_state);
    }
}

int CPU6502::step()
{
    c6502_byte_t opcode = advance();

    OpHandler oph = s_ophandlers[opcode];
    if (oph)
    {
        m_penalty = 0;
        (this->*oph)();
        return CYCLES[opcode] + m_penalty;
    }
    else
    {
        m_state = STATE_ERROR;

        Log::e("Bad opcode %X", opcode);
        assert(false && "Bad opcode");
        return 0;
    }
}
