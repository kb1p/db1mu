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

    // Get the byte PC points to and advance PC
    inline c6502_byte_t advance()
    {
        return readMem(m_regs.pc.W++);
    }

    // opcode -> unified handler prototype
    #define OPDECL(code) void op_##code();

    // define handler prototypes for all opcodes
    FOR_EACH_OPCODE(OPDECL)
};

#endif