#ifndef __OPCODES_H__
#define __OPCODES_H__

// List of 6502 opcodes
enum OpCode
{
    ADC_IMM = 0x69,
    ADC_ZP = 0x65,
    ADC_ZPX = 0x75,
    ADC_ABS = 0x6D,
    ADC_ABX = 0x7D,
    ADC_ABY = 0x79,
    ADC_INX = 0x61,
    ADC_INY = 0x71,
    AND_IMM = 0x29,
    AND_ZP = 0x25,
    AND_ZPX = 0x35,
    AND_ABS = 0x2D,
    AND_ABX = 0x3D,
    AND_ABY = 0x39,
    AND_INX = 0x21,
    AND_INY = 0x31,
    ASL_ACC = 0x0A,
    ASL_ZP = 0x06,
    ASL_ZPX = 0x16,
    ASL_ABS = 0x0E,
    ASL_ABX = 0x1E,
    BCC = 0x90,
    BCS = 0xB0,
    BEQ = 0xF0,
    BIT_ZP = 0x24,
    BIT_ABS = 0x2C,
    BMI = 0x30,
    BNE = 0xD0,
    BPL = 0x10,
    BRK = 0x00,
    BVC = 0x50,
    BVS = 0x70,
    CLC = 0x18,
    CLD = 0xD8,
    CLI = 0x58,
    CLV = 0xB8,
    CMP_IMM = 0xC9,
    CMP_ZP = 0xC5,
    CMP_ZPX = 0xD5,
    CMP_ABS = 0xCD,
    CMP_ABX = 0xDD,
    CMP_ABY = 0xD9,
    CMP_INX = 0xC1,
    CMP_INY = 0xD1,
    CPX_IMM = 0xE0,
    CPX_ZP = 0xE4,
    CPX_ABS = 0xEC,
    CPY_IMM = 0xC0,
    CPY_ZP = 0xC4,
    CPY_ABS = 0xCC,
    DEC_ZP = 0xC6,
    DEC_ZPX = 0xD6,
    DEC_ABS = 0xCE,
    DEC_ABX = 0xDE,
    DEX = 0xCA,
    DEY = 0x88,
    EOR_IMM = 0x49,
    EOR_ZP = 0x45,
    EOR_ZPX = 0x55,
    EOR_ABS = 0x4D,
    EOR_ABX = 0x5D,
    EOR_ABY = 0x59,
    EOR_INX = 0x41,
    EOR_INY = 0x51,
    INC_ZP = 0xE6,
    INC_ZPX = 0xF6,
    INC_ABS = 0xEE,
    INC_ABX = 0xFE,
    INX = 0xE8,
    INY = 0xC8,
    JMP_ABS = 0x4C,
    JMP_IND = 0x6C,
    JSR = 0x20,
    LDA_IMM = 0xA9,
    LDA_ZP = 0xA5,
    LDA_ZPX = 0xB5,
    LDA_ABS = 0xAD,
    LDA_ABX = 0xBD,
    LDA_ABY = 0xB9,
    LDA_INX = 0xA1,
    LDA_INY = 0xB1,
    LDX_IMM = 0xA2,
    LDX_ZP = 0xA6,
    LDX_ZPY = 0xB6,
    LDX_ABS = 0xAE,
    LDX_ABY = 0xBE,
    LDY_IMM = 0xA0,
    LDY_ZP = 0xA4,
    LDY_ZPY = 0xB4,
    LDY_ABS = 0xAC,
    LDY_ABY = 0xBC,
    LSR_ACC = 0x4A,
    LSR_ZP = 0x46,
    LSR_ZPX = 0x56,
    LSR_ABS = 0x4E,
    LSR_ABX = 0x5E,
    NOP = 0xEA,
    ORA_IMM = 0x09,
    ORA_ZP = 0x05,
    ORA_ZPX = 0x15,
    ORA_ABS = 0x0D,
    ORA_ABX = 0x1D,
    ORA_ABY = 0x19,
    ORA_INX = 0x01,
    ORA_INY = 0x11,
    PHA = 0x48,
    PHP = 0x08,
    PLA = 0x68,
    PLP = 0x28,
    ROL_ACC = 0x2A,
    ROL_ZP = 0x26,
    ROL_ZPX = 0x36,
    ROL_ABS = 0x2E,
    ROL_ABX = 0x3E,
    ROR_ACC = 0x6A,
    ROR_ZP = 0x66,
    ROR_ZPX = 0x76,
    ROR_ABS = 0x6E,
    ROR_ABX = 0x7E,
    RTI = 0x40,
    RTS = 0x60,
    SBC_IMM = 0xE9,
    SBC_ZP = 0xE5,
    SBC_ZPX = 0xF5,
    SBC_ABS = 0xED,
    SBC_ABX = 0xFD,
    SBC_ABY = 0xF9,
    SBC_INX = 0xE1,
    SBC_INY = 0xF1,
    SEC = 0x38,
    SED = 0xF8,
    SEI = 0x78,
    STA_ZP = 0x85,
    STA_ZPX = 0x95,
    STA_ABS = 0x8D,
    STA_ABX = 0x9D,
    STA_ABY = 0x99,
    STA_INX = 0x81,
    STA_INY = 0x91,
    STX_ZP = 0x86,
    STX_ZPY = 0x96,
    STX_ABS = 0x8E,
    STY_ZP = 0x84,
    STY_ZPX = 0x94,
    STY_ABS = 0x8C,
    TAX = 0xAA,
    TAY = 0xA8,
    TSX = 0xBA,
    TXA = 0x8A,
    TXS = 0x9A,
    TYA = 0x98,
    OPCODE_COUNT = 0xFFu
};

// Perform a generic operation for all defined opcodes
#define FOR_EACH_OPCODE(OP) OP(ADC_IMM)  \
                            OP(ADC_ZP)   \
                            OP(ADC_ZPX)  \
                            OP(ADC_ABS)  \
                            OP(ADC_ABX)  \
                            OP(ADC_ABY)  \
                            OP(ADC_INX)  \
                            OP(ADC_INY)  \
                            OP(AND_IMM)  \
                            OP(AND_ZP)   \
                            OP(AND_ZPX)  \
                            OP(AND_ABS)  \
                            OP(AND_ABX)  \
                            OP(AND_ABY)  \
                            OP(AND_INX)  \
                            OP(AND_INY)  \
                            OP(ASL_ACC)  \
                            OP(ASL_ZP)   \
                            OP(ASL_ZPX)  \
                            OP(ASL_ABS)  \
                            OP(ASL_ABX)  \
                            OP(BCC)      \
                            OP(BCS)      \
                            OP(BEQ)      \
                            OP(BIT_ZP)   \
                            OP(BIT_ABS)  \
                            OP(BMI)      \
                            OP(BNE)      \
                            OP(BPL)      \
                            OP(BRK)      \
                            OP(BVC)      \
                            OP(BVS)      \
                            OP(CLC)      \
                            OP(CLD)      \
                            OP(CLI)      \
                            OP(CLV)      \
                            OP(CMP_IMM)  \
                            OP(CMP_ZP)   \
                            OP(CMP_ZPX)  \
                            OP(CMP_ABS)  \
                            OP(CMP_ABX)  \
                            OP(CMP_ABY)  \
                            OP(CMP_INX)  \
                            OP(CMP_INY)  \
                            OP(CPX_IMM)  \
                            OP(CPX_ZP)   \
                            OP(CPX_ABS)  \
                            OP(CPY_IMM)  \
                            OP(CPY_ZP)   \
                            OP(CPY_ABS)  \
                            OP(DEC_ZP)   \
                            OP(DEC_ZPX)  \
                            OP(DEC_ABS)  \
                            OP(DEC_ABX)  \
                            OP(DEX)      \
                            OP(DEY)      \
                            OP(EOR_IMM)  \
                            OP(EOR_ZP)   \
                            OP(EOR_ZPX)  \
                            OP(EOR_ABS)  \
                            OP(EOR_ABX)  \
                            OP(EOR_ABY)  \
                            OP(EOR_INX)  \
                            OP(EOR_INY)  \
                            OP(INC_ZP)   \
                            OP(INC_ZPX)  \
                            OP(INC_ABS)  \
                            OP(INC_ABX)  \
                            OP(INX)      \
                            OP(INY)      \
                            OP(JMP_ABS)  \
                            OP(JMP_IND)  \
                            OP(JSR)      \
                            OP(LDA_IMM)  \
                            OP(LDA_ZP)   \
                            OP(LDA_ZPX)  \
                            OP(LDA_ABS)  \
                            OP(LDA_ABX)  \
                            OP(LDA_ABY)  \
                            OP(LDA_INX)  \
                            OP(LDA_INY)  \
                            OP(LDX_IMM)  \
                            OP(LDX_ZP)   \
                            OP(LDX_ZPY)  \
                            OP(LDX_ABS)  \
                            OP(LDX_ABY)  \
                            OP(LDY_IMM)  \
                            OP(LDY_ZP)   \
                            OP(LDY_ZPY)  \
                            OP(LDY_ABS)  \
                            OP(LDY_ABY)  \
                            OP(LSR_ACC)  \
                            OP(LSR_ZP)   \
                            OP(LSR_ZPX)  \
                            OP(LSR_ABS)  \
                            OP(LSR_ABX)  \
                            OP(NOP)      \
                            OP(ORA_IMM)  \
                            OP(ORA_ZP)   \
                            OP(ORA_ZPX)  \
                            OP(ORA_ABS)  \
                            OP(ORA_ABX)  \
                            OP(ORA_ABY)  \
                            OP(ORA_INX)  \
                            OP(ORA_INY)  \
                            OP(PHA)      \
                            OP(PHP)      \
                            OP(PLA)      \
                            OP(PLP)      \
                            OP(ROL_ACC)  \
                            OP(ROL_ZP)   \
                            OP(ROL_ZPX)  \
                            OP(ROL_ABS)  \
                            OP(ROL_ABX)  \
                            OP(ROR_ACC)  \
                            OP(ROR_ZP)   \
                            OP(ROR_ZPX)  \
                            OP(ROR_ABS)  \
                            OP(ROR_ABX)  \
                            OP(RTI)      \
                            OP(RTS)      \
                            OP(SBC_IMM)  \
                            OP(SBC_ZP)   \
                            OP(SBC_ZPX)  \
                            OP(SBC_ABS)  \
                            OP(SBC_ABX)  \
                            OP(SBC_ABY)  \
                            OP(SBC_INX)  \
                            OP(SBC_INY)  \
                            OP(SEC)      \
                            OP(SED)      \
                            OP(SEI)      \
                            OP(STA_ZP)   \
                            OP(STA_ZPX)  \
                            OP(STA_ABS)  \
                            OP(STA_ABX)  \
                            OP(STA_ABY)  \
                            OP(STA_INX)  \
                            OP(STA_INY)  \
                            OP(STX_ZP)   \
                            OP(STX_ZPY)  \
                            OP(STX_ABS)  \
                            OP(STY_ZP)   \
                            OP(STY_ZPX)  \
                            OP(STY_ABS)  \
                            OP(TAX)      \
                            OP(TAY)      \
                            OP(TSX)      \
                            OP(TXA)      \
                            OP(TXS)      \
                            OP(TYA)

#endif