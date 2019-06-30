#ifndef CPU6502_H
#define CPU6502_H

#include "common.h"
#include "opcodes.h"
#include "bus.h"

class CPU6502
{
    friend class Debugger;
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

    CPU6502(Mode mode, Bus &bus);

    void clock();
    void reset();
    void IRQ();
    void NMI();

    State state() const noexcept
    {
        return m_state;
    }

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
            c6502_byte_t reg;
            struct
            {
                c6502_byte_t c:1;
                c6502_byte_t z:1;
                c6502_byte_t i:1;
                c6502_byte_t d:1;
                c6502_byte_t b:1;
                c6502_byte_t unused:1;
                c6502_byte_t v:1;
                c6502_byte_t n:1;
            } flags;
        } p;
        union
        {
            struct
            {
                c6502_byte_t l, h;
            } B;
            c6502_word_t W;
        } pc;
    } m_regs;

    Mode m_mode;
    State m_state;
    int m_period;
    Bus &m_bus;
    int m_penalty;

    c6502_byte_t readMem(c6502_word_t addr)
    {
        return m_bus.read(addr);
    }

    void writeMem(c6502_word_t addr, c6502_byte_t val)
    {
        m_bus.write(addr, val);
    }

    void updateScreen();
    void testKeys();
    int step();

    // Helpers
    // Push to / pop from the stack shorthands
    void push(c6502_byte_t v)
    {
        writeMem(0x100 | m_regs.s--, v);
    }

    c6502_byte_t pop()
    {
        return readMem(0x100 | (++m_regs.s));
    }

    // Get the byte PC points to and increase PC by 1
    c6502_byte_t advance()
    {
        return readMem(m_regs.pc.W++);
    }

    /*** Operand fetching routines ***/
    c6502_byte_t fetchImmOp()
    {
        return readMem(m_regs.pc.W++);
    }

    c6502_word_t fetchZPAddr()
    {
        return readMem(m_regs.pc.W++);
    }

    c6502_byte_t fetchZPOp()
    {
        return readMem(fetchZPAddr());
    }

    c6502_word_t fetchZPXAddr()
    {
        const c6502_word_t addr = readMem(m_regs.pc.W++) + m_regs.x;
        return addr > 0xFFu ? 0 : addr;
    }

    c6502_byte_t fetchZPXOp()
    {
        return readMem(fetchZPXAddr());
    }

    c6502_word_t fetchZPYAddr()
    {
        const c6502_word_t addr = readMem(m_regs.pc.W++) + m_regs.y;
        return addr > 0xFFu ? 0 : addr;
    }

    c6502_byte_t fetchZPYOp()
    {
        return readMem(fetchZPYAddr());
    }

    c6502_word_t fetchABSAddr()
    {
        const c6502_word_t al = readMem(m_regs.pc.W++),
                           ah = readMem(m_regs.pc.W++);
        return al | (ah << 8);
    }

    c6502_byte_t fetchABSOp()
    {
        return readMem(fetchABSAddr());
    }

    c6502_word_t fetchABXAddr()
    {
        const c6502_word_t al = readMem(m_regs.pc.W++),
                           ah = readMem(m_regs.pc.W++);

        // Page bound crossing check: if the lsb + index affects msb
        m_penalty = (al + m_regs.x > 0xFFu) ? 1 : 0;

        return (al | (ah << 8)) + m_regs.x;
    }

    c6502_byte_t fetchABXOp()
    {
        return readMem(fetchABXAddr());
    }

    c6502_word_t fetchABYAddr()
    {
        const c6502_word_t al = readMem(m_regs.pc.W++),
                           ah = readMem(m_regs.pc.W++);

        m_penalty = (al + m_regs.y > 0xFFu) ? 1 : 0;

        return (al | (ah << 8)) + m_regs.y;
    }

    c6502_byte_t fetchABYOp()
    {
        return readMem(fetchABYAddr());
    }

    c6502_word_t fetchINXAddr()
    {
        const c6502_word_t baddr = (readMem(m_regs.pc.W++) + m_regs.x) & 0xFFu,
                           laddr = readMem(baddr),
                           haddr = readMem((baddr + 1) & 0xFFu);
        return laddr | (haddr << 8);
    }

    c6502_byte_t fetchINXOp()
    {
        return readMem(fetchINXAddr());
    }

    c6502_word_t fetchINYAddr()
    {
        const c6502_d_word_t baddr = readMem(m_regs.pc.W++),
                             laddr = readMem(baddr),
                             haddr = readMem((baddr + 1) & 0xFFu);

        m_penalty = (laddr + m_regs.y > 0xFFu) ? 1 : 0;

        return (laddr | (haddr << 8)) + m_regs.y;
    }

    c6502_byte_t fetchINYOp()
    {
        return readMem(fetchINYAddr());
    }

    void branchIF(bool expression)
    {
        if (expression)
        {
            m_penalty = 1;
            const c6502_byte_t oldPC_h = (m_regs.pc.W - 1) >> 8;
            const auto dis = static_cast<c6502_reldis_t>(fetchImmOp());
            m_regs.pc.W = static_cast<c6502_word_t>(static_cast<int>(m_regs.pc.W) + dis);
            if (oldPC_h != m_regs.pc.B.h)
                m_penalty = 2;
        }
        else
            ++m_regs.pc.W;
    }

    // opcode -> unified handler prototype
    #define OPDECL(code) void op_##code();

    // define handler prototypes for all opcodes
    FOR_EACH_OPCODE(OPDECL)
};

#endif
