#ifndef CPU6502_H
#define CPU6502_H

#include "common.h"

// 6502 opcodes
#define ADC_IMM 0x69
#define ADC_ZP  0x65
#define ADC_ZPX 0x75
#define ADC_ABS 0x6D
#define ADC_ABX 0x7D
#define ADC_ABY 0x79
#define ADC_INX 0x61
#define ADC_INY 0x71
#define AND_IMM 0x29
#define AND_ZP  0x25
#define AND_ZPX 0x35
#define AND_ABS 0x2D
#define AND_ABX 0x3D
#define AND_ABY 0x39
#define AND_INX 0x21
#define AND_INY 0x31
#define ASL_ACC 0x0A
#define ASL_ZP  0x06
#define ASL_ZPX 0x16
#define ASL_ABS 0x0E
#define ASL_ABX 0x1E
#define BCC     0x90
#define BCS     0xB0
#define BEQ     0xF0
#define BIT_ZP  0x24
#define BIT_ABS 0x2C
#define BMI     0x30
#define BNE     0xD0
#define BPL     0x10
#define BRK     0x00
#define BVC     0x50
#define BVS     0x70
#define CLC     0x18
#define CLD     0xD8
#define CLI     0x58
#define CLV     0xB8
#define CMP_IMM 0xC9
#define CMP_ZP  0xC5
#define CMP_ZPX 0xD5
#define CMP_ABS 0xCD
#define CMP_ABX 0xDD
#define CMP_ABY 0xD9
#define CMP_INX 0xC1
#define CMP_INY 0xD1
#define CPX_IMM 0xE0
#define CPX_ZP  0xE4
#define CPX_ABS 0xEC
#define CPY_IMM 0xC0
#define CPY_ZP  0xC4
#define CPY_ABS 0xCC
#define DEC_ZP  0xC6
#define DEC_ZPX 0xD6
#define DEC_ABS 0xCE
#define DEC_ABX 0xDE
#define DEX     0xCA
#define DEY     0x88
#define EOR_IMM 0x49
#define EOR_ZP  0x45
#define EOR_ZPX 0x55
#define EOR_ABS 0x4D
#define EOR_ABX 0x5D
#define EOR_ABY 0x59
#define EOR_INX 0x41
#define EOR_INY 0x51
#define INC_ZP  0xE6
#define INC_ZPX 0xF6
#define INC_ABS 0xEE
#define INC_ABX 0xFE
#define INX     0xE8
#define INY     0xC8
#define JMP_ABS 0x4C
#define JMP_IND 0x6C
#define JSR     0x20
#define LDA_IMM 0xA9
#define LDA_ZP  0xA5
#define LDA_ZPX 0xB5
#define LDA_ABS 0xAD
#define LDA_ABX 0xBD
#define LDA_ABY 0xB9
#define LDA_INX 0xA1
#define LDA_INY 0xB1
#define LDX_IMM 0xA2
#define LDX_ZP  0xA6
#define LDX_ZPY 0xB6
#define LDX_ABS 0xAE
#define LDX_ABY 0xBE
#define LDY_IMM 0xA0
#define LDY_ZP  0xA4
#define LDY_ZPY 0xB4
#define LDY_ABS 0xAC
#define LDY_ABY 0xBC
#define LSR_ACC 0x4A
#define LSR_ZP  0x46
#define LSR_ZPX 0x56
#define LSR_ABS 0x4E
#define LSR_ABX 0x5E
#define NOP     0xEA
#define ORA_IMM 0x09
#define ORA_ZP  0x05
#define ORA_ZPX 0x15
#define ORA_ABS 0x0D
#define ORA_ABX 0x1D
#define ORA_ABY 0x19
#define ORA_INX 0x01
#define ORA_INY 0x11
#define PHA     0x48
#define PHP     0x08
#define PLA     0x68
#define PLP     0x28
#define ROL_ACC 0x2A
#define ROL_ZP  0x26
#define ROL_ZPX 0x36
#define ROL_ABS 0x2E
#define ROL_ABX 0x3E
#define ROR_ACC 0x6A
#define ROR_ZP  0x66
#define ROR_ZPX 0x76
#define ROR_ABS 0x6E
#define ROR_ABX 0x7E
#define RTI     0x40
#define RTS     0x60
#define SBC_IMM 0xE9
#define SBC_ZP  0xE5
#define SBC_ZPX 0xF5
#define SBC_ABS 0xED
#define SBC_ABX 0xFD
#define SBC_ABY 0xF9
#define SBC_INX 0xE1
#define SBC_INY 0xF1
#define SEC     0x38
#define SED     0xF8
#define SEI     0x78
#define STA_ZP  0x85
#define STA_ZPX 0x95
#define STA_ABS 0x8D
#define STA_ABX 0x9D
#define STA_ABY 0x99
#define STA_INX 0x81
#define STA_INY 0x91
#define STX_ZP  0x86
#define STX_ZPY 0x96
#define STX_ABS 0x8E
#define STY_ZP  0x84
#define STY_ZPX 0x94
#define STY_ABS 0x8C
#define TAX     0xAA
#define TAY     0xA8
#define TSX     0xBA
#define TXA     0x8A
#define TXS     0x9A
#define TYA     0x98
#define OPCODE_COUNT 0xFFu

class CPU6502
{
public:
    enum Mode
    {
        PAL = 0, NTSC
    };

    enum RunState
    {
        STATE_HALTED,
        STATE_RUN,
        STATE_ERROR
    };

    // CPU state which can be shared with non-members
    struct State
    {
        /* Layout:
         * - accumulator
         * - flags
         * - X, Y indexes
         * - stack pointer
         * - program counter
         */
        struct Reg
        {
            c6502_byte_t a, p, x, y, s;
            union
            {
                struct
                {
                    c6502_byte_t l, h;
                } B;
                c6502_word_t W;
            } pc;
        } regs;

        /*** 6502 MEMORY MAP ***/
        // Internal RAM: 0x0000 ~ 0x2000.
        // 0x0000 ~ 0x0100 is a z-page, have special meaning for addressing.
        c6502_byte_t ram[0x800];

        // Cartridge RAM: 0x6000 ~ 0x8000
        c6502_byte_t wram[0x2000];

        // Catridge ROM may contain from 32 to 256kb of 2kb-sized pages
        // Basic addressed are 0x8000 ~ 0xFFFF; addresses 0x8000 ~ 0xC000 are a switchable banks
        c6502_byte_t rom[128][0x2000];
    };

    CPU6502(Mode mode);
    void Clock();
    void reset();

private:
    typedef void (*OpHandler)(State*);

    // Opcode handlers table
    static OpHandler m_ophandlers[OPCODE_COUNT];

    State m_s;

    Mode m_mode;
    RunState m_rstate;
    int m_period;

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
        writeMem(0x100 | m_s.regs.s--, v);
    }

    inline c6502_byte_t pop()
    {
        return readMem(0x100 | (++m_s.regs.s));
    }

    // Get the byte PC points to and advance PC
    inline c6502_byte_t advance()
    {
        return readMem(m_s.regs.pc.W++);
    }
};

#endif