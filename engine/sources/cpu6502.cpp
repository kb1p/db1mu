/*
 * 6502 CPU emulation routines.
 *
 * TODO: replace all these magic numbers with #defines at least.
 */

#include "cpu6502.h"
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

/*
 * 6502 OPCODE IMPLEMENTATION
 */
void op_nop(CPU6502::State*)
{
    // just eat the tacts
}

void op_brk(CPU6502::State *s)
{
    /*
     *   cpu_regs.pc.W++;
     *   PUSH(cpu_regs.pc.W >> 8);
     *   PUSH(_PC);
     *   PUSH(_P|U_FLAG|B_FLAG);
     * 
     *   _P|=I_FLAG;
     *   _PI|=I_FLAG;
     *   _PC=RdMem(0xFFFE);
     *   _PC|=RdMem(0xFFFF)<<8;
     */
}

/*** CPU class implementation ***/
CPU6502::CPU6502(Mode mode):
    m_mode(mode),
    m_rstate(STATE_HALTED),
    m_period(0)
{
    for (int i = 0; i < OPCODE_COUNT; i++)
        m_ophandlers[i] = nullptr;

#define ASSIGN(opcode, name) m_ophandlers[(opcode)] = &(name);

    // Initialize opcode handler table
    ASSIGN(NOP, op_nop)
    ASSIGN(BRK, op_brk)

#undef ASSIGN
}


c6502_byte_t CPU6502::readMem(c6502_word_t addr)
{
    switch (addr >> 13)
    {
        case 0:
            return m_s.ram[addr & 0x7FF];
        case 3:
            return m_s.wram[addr & 0x1FFF];
        case 1:
        case 2:
            return readIO(addr);
        default:
            addr &= 0x7FFF;
            return m_s.rom[addr >> 13][addr];
    }
}

void CPU6502::writeMem(c6502_word_t addr, c6502_byte_t val)
{
    switch (addr >> 13)
    {
        case 0:
            // To internal RAM
            m_s.ram[addr & 0x7FF] = val;
            break;
        case 3:
            // To cartridge RAM
            m_s.wram[addr & 0x1FFF] = val;
            break;
        case 1:
        case 2:
            // To GPU or APU registers
            writeIO(addr, val);
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
    m_s.regs.a = m_s.regs.x = m_s.regs.y = 0;
    m_s.regs.p = 0x22;
    m_s.regs.s = 0xFF;
    m_s.regs.pc.B.l = readMem(0xFFFC);
    m_s.regs.pc.B.h = readMem(0xFFFD);
    m_period = TPP[m_mode];
    m_rstate = STATE_RUN;
}

// Handle maskable interrupt
void CPU6502::IRQ()
{

}

// Handle non-maskable interrupt
void CPU6502::NMI()
{
    push(m_s.regs.pc.B.h);
    push(m_s.regs.pc.B.l);
    push(m_s.regs.p);

    m_s.regs.pc.B.l = readMem(0xFFFA);
    m_s.regs.pc.B.h = readMem(0xFFFB);

    m_period -= 7;
}

void CPU6502::Clock()
{
    if (m_rstate == STATE_RUN)
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

    OpHandler oph = m_ophandlers[opcode];
    if (oph)
    {
        oph(&m_s);
        return CYCLES[opcode];
    }
    else
    {
        m_rstate = STATE_ERROR;

        // TODO: add error handling
        return 0;
    }
}
