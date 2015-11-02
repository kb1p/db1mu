#ifndef CPU6502_H
#define CPU6502_H

#include "common.h"
#include "storage.h"
#include "opcodes.h"

struct Cartrige;
struct PPU;

class CPU6502
{
public:
    enum Mode
    {
        PAL = 0, NTSC
    };

    enum State
    {
        STATE_HALTED,
        STATE_RUN,
        STATE_ERROR
    };

    CPU6502(Mode mode);
    void InjectCartrige(Cartrige*);
    void Clock();
    void reset();

private:
    typedef void (CPU6502::*OpHandler)(void);

    // Opcode handlers table
    static OpHandler s_ophandlers[];

    /* Layout:
     * - accumulator
     * - flags
     * - X, Y indexes
     * - stack pointer
     * - program counter
     */
    struct Reg
    {
        c6502_byte_t a, x, y, s;
        union
        {
            struct
            {
                unsigned C: 1;
                unsigned Z: 1;
                unsigned I: 1;
                unsigned D: 1;
                unsigned B: 1;
                unsigned  : 1;
                unsigned V: 1;
                unsigned N: 1;
            } pbit;
            c6502_byte_t p;
        };
        union
        {
            struct
            {
                c6502_byte_t l, h;
            } B;
            c6502_word_t W;
        } pc;
    } m_regs;

    /*** 6502 MEMORY MAP ***/
    // Internal RAM: 0x0000 ~ 0x2000.
    // 0x0000 ~ 0x0100 is a z-page, have special meaning for addressing.
    Storage<0x800> m_ram;

    Mode m_mode;
    State m_state;
    int m_period;
    Cartrige* m_activeCartrige;
    PPU* m_ppu;

    c6502_byte_t readMem(c6502_word_t addr);
    void writeMem(c6502_word_t addr, c6502_byte_t val);
    c6502_byte_t readIO(c6502_word_t addr);
    void writeIO(c6502_word_t addr, c6502_byte_t val);
    c6502_byte_t readMMC(c6502_word_t addr);
    void writeMMC(c6502_word_t addr, c6502_byte_t val);
    void updateScreen();
    void testKeys();
    void IRQ();
    void NMI();
    c6502_byte_t step();

    // Helpers
    // Push to / pop from the stack shorthands
    inline void push(c6502_byte_t v)
    {
        writeMem(0x100 | m_regs.s--, v);
    }

    inline c6502_byte_t pop()
    {
        return readMem(0x100 | (++m_regs.s));
    }

    // Get the byte PC points to and increase PC by 1
    inline c6502_byte_t advance()
    {
        return readMem(m_regs.pc.W++);
    }

    /*** Operand fetching routines ***/
    inline c6502_byte_t fetchImm()
    {
        return readMem(m_regs.pc.W++);
    }

    inline c6502_byte_t fetchZP()
    {
        const c6502_word_t addr = readMem(m_regs.pc.W++);
        return readMem(addr);
    }

    inline c6502_byte_t fetchZPX()
    {
        const c6502_word_t addr = readMem(m_regs.pc.W++) + m_regs.x;
        return readMem(addr > 0xFFu ? 0 : addr);
    }

    inline c6502_byte_t fetchABS()
    {
        const c6502_word_t al = readMem(m_regs.pc.W++),
                           ah = readMem(m_regs.pc.W++);
        return readMem(al | (ah << 8));
    }

    inline c6502_byte_t fetchABX()
    {
        const c6502_word_t al = readMem(m_regs.pc.W++),
                           ah = readMem(m_regs.pc.W++);
        return readMem((al | (ah << 8)) + m_regs.x);
    }

    inline c6502_byte_t fetchABY()
    {
        const c6502_word_t al = readMem(m_regs.pc.W++),
                           ah = readMem(m_regs.pc.W++);
        return readMem((al | (ah << 8)) + m_regs.y);
    }

    inline c6502_byte_t fetchINX()
    {
        const c6502_word_t baddr = (readMem(m_regs.pc.W++) + m_regs.x) & 0xFFu,
                           laddr = readMem(baddr),
                           haddr = readMem((baddr + 1) & 0xFFu);
        return readMem(laddr | (haddr << 8));
    }

    inline c6502_byte_t fetchINY()
    {
        const c6502_d_word_t baddr = readMem(m_regs.pc.W++),
                             laddr = readMem(baddr),
                             haddr = readMem((baddr + 1) & 0xFFu);
        return readMem((laddr | (haddr << 8)) + m_regs.y);
    }

    // opcode -> unified handler prototype
    #define OPDECL(code) void op_##code();

    // define handler prototypes for all opcodes
    FOR_EACH_OPCODE(OPDECL)
};

#endif