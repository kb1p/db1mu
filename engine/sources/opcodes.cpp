#include "cpu6502.h"

void CPU6502::op_NOP()
{
    // Nothing to do, just waste tacts
}

void CPU6502::op_ADC_IMM()
{
    // c6502_byte_t imm = readMem(...);
}

void CPU6502::op_ADC_ZP()
{

}

void CPU6502::op_ADC_ZPX() { }
void CPU6502::op_ADC_ABS() { }
void CPU6502::op_ADC_ABX() { }
void CPU6502::op_ADC_ABY() { }
void CPU6502::op_ADC_INX() { }
void CPU6502::op_ADC_INY() { }
void CPU6502::op_AND_IMM() { }
void CPU6502::op_AND_ZP() { }
void CPU6502::op_AND_ZPX() { }
void CPU6502::op_AND_ABS() { }
void CPU6502::op_AND_ABX() { }
void CPU6502::op_AND_ABY() { }
void CPU6502::op_AND_INX() { }
void CPU6502::op_AND_INY() { }
void CPU6502::op_ASL_ACC() { }
void CPU6502::op_ASL_ZP() { }
void CPU6502::op_ASL_ZPX() { }
void CPU6502::op_ASL_ABS() { }
void CPU6502::op_ASL_ABX() { }
void CPU6502::op_BCC() { }
void CPU6502::op_BCS() { }
void CPU6502::op_BEQ() { }
void CPU6502::op_BIT_ZP() { }
void CPU6502::op_BIT_ABS() { }
void CPU6502::op_BMI() { }
void CPU6502::op_BNE() { }
void CPU6502::op_BPL() { }
void CPU6502::op_BRK() { }
void CPU6502::op_BVC() { }
void CPU6502::op_BVS() { }
void CPU6502::op_CLC()
{
    m_regs.p.flags.c = 0;
}
void CPU6502::op_CLD()
{
    m_regs.p.flags.d = 0;
}
void CPU6502::op_CLI()
{
    m_regs.p.flags.i = 0;
}
void CPU6502::op_CLV()
{
    m_regs.p.flags.v = 0;
}
void CPU6502::op_CMP_IMM() { }
void CPU6502::op_CMP_ZP() { }
void CPU6502::op_CMP_ZPX() { }
void CPU6502::op_CMP_ABS() { }
void CPU6502::op_CMP_ABX() { }
void CPU6502::op_CMP_ABY() { }
void CPU6502::op_CMP_INX() { }
void CPU6502::op_CMP_INY() { }
void CPU6502::op_CPX_IMM() { }
void CPU6502::op_CPX_ZP() { }
void CPU6502::op_CPX_ABS() { }
void CPU6502::op_CPY_IMM() { }
void CPU6502::op_CPY_ZP() { }
void CPU6502::op_CPY_ABS() { }
void CPU6502::op_DEC_ZP() { }
void CPU6502::op_DEC_ZPX() { }
void CPU6502::op_DEC_ABS() { }
void CPU6502::op_DEC_ABX() { }
void CPU6502::op_DEX() { }
void CPU6502::op_DEY() { }
void CPU6502::op_EOR_IMM() { }
void CPU6502::op_EOR_ZP() { }
void CPU6502::op_EOR_ZPX() { }
void CPU6502::op_EOR_ABS() { }
void CPU6502::op_EOR_ABX() { }
void CPU6502::op_EOR_ABY() { }
void CPU6502::op_EOR_INX() { }
void CPU6502::op_EOR_INY() { }
void CPU6502::op_INC_ZP() { }
void CPU6502::op_INC_ZPX() { }
void CPU6502::op_INC_ABS() { }
void CPU6502::op_INC_ABX() { }
void CPU6502::op_INX() { }
void CPU6502::op_INY() { }
void CPU6502::op_JMP_ABS() { }
void CPU6502::op_JMP_IND() { }
void CPU6502::op_JSR() { }
void CPU6502::op_LDA_IMM() { }
void CPU6502::op_LDA_ZP() { }
void CPU6502::op_LDA_ZPX() { }
void CPU6502::op_LDA_ABS() { }
void CPU6502::op_LDA_ABX() { }
void CPU6502::op_LDA_ABY() { }
void CPU6502::op_LDA_INX() { }
void CPU6502::op_LDA_INY() { }
void CPU6502::op_LDX_IMM() { }
void CPU6502::op_LDX_ZP() { }
void CPU6502::op_LDX_ZPY() { }
void CPU6502::op_LDX_ABS() { }
void CPU6502::op_LDX_ABY() { }
void CPU6502::op_LDY_IMM() { }
void CPU6502::op_LDY_ZP() { }
void CPU6502::op_LDY_ZPY() { }
void CPU6502::op_LDY_ABS() { }
void CPU6502::op_LDY_ABY() { }
void CPU6502::op_LSR_ACC() { }
void CPU6502::op_LSR_ZP() { }
void CPU6502::op_LSR_ZPX() { }
void CPU6502::op_LSR_ABS() { }
void CPU6502::op_LSR_ABX() { }
void CPU6502::op_ORA_IMM() { }
void CPU6502::op_ORA_ZP() { }
void CPU6502::op_ORA_ZPX() { }
void CPU6502::op_ORA_ABS() { }
void CPU6502::op_ORA_ABX() { }
void CPU6502::op_ORA_ABY() { }
void CPU6502::op_ORA_INX() { }
void CPU6502::op_ORA_INY() { }

void CPU6502::op_PHA()
{
   push(m_regs.a);
}

void CPU6502::op_PHP()
{
    push(m_regs.p.reg);
}
void CPU6502::op_PLA()
{
    m_regs.a = pop();
    m_regs.p.flags.z = (!m_regs.a);
    m_regs.p.flags.n = (m_regs.a >> 7);
}

void CPU6502::op_PLP()
{
    m_regs.p.reg = pop();
}
void CPU6502::op_ROL_ACC() { }
void CPU6502::op_ROL_ZP() { }
void CPU6502::op_ROL_ZPX() { }
void CPU6502::op_ROL_ABS() { }
void CPU6502::op_ROL_ABX() { }
void CPU6502::op_ROR_ACC() { }
void CPU6502::op_ROR_ZP() { }
void CPU6502::op_ROR_ZPX() { }
void CPU6502::op_ROR_ABS() { }
void CPU6502::op_ROR_ABX() { }
void CPU6502::op_RTI()
{
    m_regs.p.reg = pop();
    m_regs.pc.B.l = pop();
    m_regs.pc.B.h = pop();
}
void CPU6502::op_RTS() { }
void CPU6502::op_SBC_IMM() { }
void CPU6502::op_SBC_ZP() { }
void CPU6502::op_SBC_ZPX() { }
void CPU6502::op_SBC_ABS() { }
void CPU6502::op_SBC_ABX() { }
void CPU6502::op_SBC_ABY() { }
void CPU6502::op_SBC_INX() { }
void CPU6502::op_SBC_INY() { }
void CPU6502::op_SEC()
{
    m_regs.p.flags.c = 1;
}

void CPU6502::op_SED()
{
    m_regs.p.flags.d = 1;
}
void CPU6502::op_SEI()
{
    m_regs.p.flags.i = 1;
}
void CPU6502::op_STA_ZP() { }
void CPU6502::op_STA_ZPX() { }
void CPU6502::op_STA_ABS() { }
void CPU6502::op_STA_ABX() { }
void CPU6502::op_STA_ABY() { }
void CPU6502::op_STA_INX() { }
void CPU6502::op_STA_INY() { }
void CPU6502::op_STX_ZP() { }
void CPU6502::op_STX_ZPY() { }
void CPU6502::op_STX_ABS() { }
void CPU6502::op_STY_ZP() { }
void CPU6502::op_STY_ZPX() { }
void CPU6502::op_STY_ABS() { }
void CPU6502::op_TAX() { }
void CPU6502::op_TAY() { }
void CPU6502::op_TSX() { }
void CPU6502::op_TXA() { }
void CPU6502::op_TXS() { }
void CPU6502::op_TYA() { }