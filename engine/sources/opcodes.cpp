#include "cpu6502.h"

#define HI(x) (((x) >> 8) & 0xFFu)
#define LO(x) ((x) & 0xFFu)

#define F_C (m_regs.p.flags.c)
#define F_Z (m_regs.p.flags.z)
#define F_N (m_regs.p.flags.n)
#define F_V (m_regs.p.flags.v)
#define F_D (m_regs.p.flags.d)
#define F_B (m_regs.p.flags.b)
#define F_I (m_regs.p.flags.i)

#define EVAL_C(r) (m_regs.p.flags.c = ((r) & 0x100u) >> 8)
#define EVAL_Z(r) (m_regs.p.flags.z = (r) == 0)
#define EVAL_N(r) (m_regs.p.flags.n = ((r) >> 7) & 0x1u)
#define EVAL_V(r) (m_regs.p.flags.v = (c6502_test_t(r) > 127 || c6502_test_t(r) < -128 ? 1 : 0))

#define FLAG_C 0x01
#define FLAG_Z 0x02
#define FLAG_I 0x04
#define FLAG_D 0x08
#define FLAG_B 0x10
#define FLAG_U 0x20
#define FLAG_V 0x40
#define FLAG_N 0x80

void CPU6502::op_NOP()
{
    // Nothing to do, just waste tacts
}

// Immediate operand (follows opcode in memory)
void CPU6502::op_ADC_IMM()
{
    const c6502_word_t r = m_regs.a + F_C + fetchImmOp();

    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
    EVAL_V(r);

    m_regs.a = LO(r);
}

// Zero-page addressing (byte-size address)
void CPU6502::op_ADC_ZP()
{
    const c6502_word_t r = m_regs.a + F_C + fetchZPOp();

    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
    EVAL_V(r);

    m_regs.a = LO(r);
}

// Zero-page indexed by X register
void CPU6502::op_ADC_ZPX()
{
    const c6502_word_t r = m_regs.a + F_C + fetchZPXOp();

    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
    EVAL_V(r);

    m_regs.a = LO(r);
}

// Absolute addressing (word-size address)
void CPU6502::op_ADC_ABS()
{
    const c6502_word_t r = m_regs.a + F_C + fetchABSOp();

    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
    EVAL_V(r);

    m_regs.a = LO(r);
}

// Absolute indexed by X register
void CPU6502::op_ADC_ABX()
{
    const c6502_word_t r = m_regs.a + F_C + fetchABXOp();

    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
    EVAL_V(r);

    m_regs.a = LO(r);
}

// Absolute indexed by Y register
void CPU6502::op_ADC_ABY()
{
    const c6502_word_t r = m_regs.a + F_C + fetchABYOp();

    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
    EVAL_V(r);

    m_regs.a = LO(r);
}

void CPU6502::op_ADC_INX()
{
    const c6502_word_t r = m_regs.a + F_C + fetchINXOp();

    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
    EVAL_V(r);

    m_regs.a = LO(r);
}

void CPU6502::op_ADC_INY()
{
    const c6502_word_t r = m_regs.a + F_C + fetchINYOp();

    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
    EVAL_V(r);

    m_regs.a = LO(r);
}

void CPU6502::op_AND_IMM()
{
    m_regs.a &= fetchImmOp();

    EVAL_Z(m_regs.a);
    EVAL_N(m_regs.a);
}
void CPU6502::op_AND_ZP()
{
    m_regs.a &= fetchZPOp();

    EVAL_Z(m_regs.a);
    EVAL_N(m_regs.a);
}
void CPU6502::op_AND_ZPX()
{
    m_regs.a &= fetchZPXOp();

    EVAL_Z(m_regs.a);
    EVAL_N(m_regs.a);
}
void CPU6502::op_AND_ABS()
{
    m_regs.a &= fetchABSOp();

    EVAL_Z(m_regs.a);
    EVAL_N(m_regs.a);
}
void CPU6502::op_AND_ABX()
{
    m_regs.a &= fetchABXOp();

    EVAL_Z(m_regs.a);
    EVAL_N(m_regs.a);
}
void CPU6502::op_AND_ABY()
{
    m_regs.a &= fetchABYOp();

    EVAL_Z(m_regs.a);
    EVAL_N(m_regs.a);
}
void CPU6502::op_AND_INX()
{
    m_regs.a &= fetchINXOp();

    EVAL_Z(m_regs.a);
    EVAL_N(m_regs.a);
}
void CPU6502::op_AND_INY()
{
    m_regs.a &= fetchINYOp();

    EVAL_Z(m_regs.a);
    EVAL_N(m_regs.a);
}
void CPU6502::op_ASL_ACC()
{
    F_C = 0x80u & m_regs.a;
    m_regs.a <<= 1;

    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_ASL_ZP()
{
    const c6502_word_t addr = fetchZPAddr();
    c6502_byte_t op = readMem(addr);
    F_C = 0x80u & op;
    op <<= 1;

    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_ASL_ZPX()
{
    const c6502_word_t addr = fetchZPXAddr();
    c6502_byte_t op = readMem(addr);
    F_C = 0x80u & op;
    op <<= 1;

    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_ASL_ABS()
{
    const c6502_word_t addr = fetchABSAddr();
    c6502_byte_t op = readMem(addr);
    F_C = 0x80u & op;
    op <<= 1;

    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_ASL_ABX()
{
    const c6502_word_t addr = fetchABXAddr();
    c6502_byte_t op = readMem(addr);
    F_C = 0x80u & op;
    op <<= 1;

    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_BCC()
{
    branchIF(! m_regs.p.flags.c);
}
void CPU6502::op_BCS()
{
    branchIF(m_regs.p.flags.c);
}
void CPU6502::op_BEQ()
{
    branchIF(m_regs.p.flags.z);
}
void CPU6502::op_BIT_ZP()
{
    const c6502_byte_t r = m_regs.a & fetchZPOp();

    EVAL_Z(r);
    EVAL_N(r);
    F_V = (r >> 6) & 0x1u;
}
void CPU6502::op_BIT_ABS()
{
    const c6502_byte_t r = m_regs.a & fetchABSOp();

    EVAL_N(r);
    EVAL_Z(r);
    F_V = (r >> 6) & 0x1u;
}
void CPU6502::op_BMI()
{
    branchIF(m_regs.p.flags.n);
}
void CPU6502::op_BNE()
{
    branchIF(! m_regs.p.flags.z);
}
void CPU6502::op_BPL()
{
    branchIF(! m_regs.p.flags.n);
}
void CPU6502::op_BRK()
{
    push(m_regs.pc.B.h);
    push(m_regs.pc.B.l);
    push(m_regs.p.reg | FLAG_B);
    m_regs.p.flags.i = 1;

    m_regs.pc.B.l = readMem(0xFFFE);
    m_regs.pc.B.h = readMem(0xFFFF);
}
void CPU6502::op_BVC()
{
    branchIF(! m_regs.p.flags.v);
}
void CPU6502::op_BVS()
{
    branchIF(m_regs.p.flags.v);
}
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
void CPU6502::op_CMP_IMM()
{
    // Type promotion to a signed 2-byte int is important for CMP
    c6502_test_t r = m_regs.a;
    r -= fetchImmOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CMP_ZP()
{
    c6502_test_t r = m_regs.a;
    r -= fetchZPOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}

void CPU6502::op_CMP_ZPX()
{
    c6502_test_t r = m_regs.a;
    r -= fetchZPXOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CMP_ABS()
{
    c6502_test_t r = m_regs.a;
    r -= fetchABSOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CMP_ABX()
{
    c6502_test_t r = m_regs.a;
    r -= fetchABXOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CMP_ABY()
{
    c6502_test_t r = m_regs.a;
    r -= fetchABYOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CMP_INX()
{
    c6502_test_t r = m_regs.a;
    r -= fetchINXOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CMP_INY()
{
    c6502_test_t r = m_regs.a;
    r -= fetchINYOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CPX_IMM()
{
    c6502_test_t r = m_regs.x;
    r -= fetchImmOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CPX_ZP()
{
    c6502_test_t r = m_regs.x;
    r -= fetchZPOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CPX_ABS()
{
    c6502_test_t r = m_regs.x;
    r -= fetchABSOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CPY_IMM()
{
    c6502_test_t r = m_regs.y;
    r -= fetchImmOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CPY_ZP()
{
    c6502_test_t r = m_regs.y;
    r -= fetchZPOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_CPY_ABS()
{
    c6502_test_t r = m_regs.y;
    r -= fetchABSOp();
    EVAL_C(r);
    EVAL_Z(r);
    EVAL_N(r);
}
void CPU6502::op_DEC_ZP()
{
    const c6502_word_t addr = fetchZPAddr();
    const c6502_byte_t op = readMem(addr) - 1;
    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_DEC_ZPX()
{
    const c6502_word_t addr = fetchZPXAddr();
    const c6502_byte_t op = readMem(addr) - 1;
    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_DEC_ABS()
{
    const c6502_word_t addr = fetchABSAddr();
    const c6502_byte_t op = readMem(addr) - 1;
    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_DEC_ABX()
{
    const c6502_word_t addr = fetchABXAddr();
    const c6502_byte_t op = readMem(addr) - 1;
    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_DEX()
{
    m_regs.x--;
    EVAL_N(m_regs.x);
    EVAL_Z(m_regs.x);
}
void CPU6502::op_DEY()
{
    m_regs.y--;
    EVAL_N(m_regs.y);
    EVAL_Z(m_regs.y);
}
void CPU6502::op_EOR_IMM()
{
    const c6502_byte_t op = fetchImmOp();
    m_regs.a ^= op;
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_EOR_ZP()
{
    const c6502_byte_t op = fetchZPOp();
    m_regs.a ^= op;
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_EOR_ZPX()
{
    const c6502_byte_t op = fetchZPXOp();
    m_regs.a ^= op;
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_EOR_ABS()
{
    const c6502_byte_t op = fetchABSOp();
    m_regs.a ^= op;
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_EOR_ABX()
{
    const c6502_byte_t op = fetchABXOp();
    m_regs.a ^= op;
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_EOR_ABY()
{
    const c6502_byte_t op = fetchABYOp();
    m_regs.a ^= op;
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_EOR_INX()
{
    const c6502_byte_t op = fetchINXOp();
    m_regs.a ^= op;
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_EOR_INY()
{
    const c6502_byte_t op = fetchINYOp();
    m_regs.a ^= op;
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_INC_ZP()
{
    const c6502_word_t addr = fetchZPAddr();
    const c6502_byte_t op = readMem(addr) + 1;
    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_INC_ZPX()
{
    const c6502_word_t addr = fetchZPXAddr();
    const c6502_byte_t op = readMem(addr) + 1;
    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_INC_ABS()
{
    const c6502_word_t addr = fetchABSAddr();
    const c6502_byte_t op = readMem(addr) + 1;
    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_INC_ABX()
{
    const c6502_word_t addr = fetchABXAddr();
    const c6502_byte_t op = readMem(addr) + 1;
    EVAL_N(op);
    EVAL_Z(op);
    writeMem(addr, op);
}
void CPU6502::op_INX()
{
    m_regs.x++;
    EVAL_N(m_regs.x);
    EVAL_Z(m_regs.x);
}
void CPU6502::op_INY()
{
    m_regs.y++;
    EVAL_N(m_regs.y);
    EVAL_Z(m_regs.y);
}
void CPU6502::op_JMP_ABS()
{
    m_regs.pc.W = fetchABSOp();
}
void CPU6502::op_JMP_IND()
{
    c6502_word_t ptr = fetchABSOp();
    m_regs.pc.B.l = readMem(ptr);
    m_regs.pc.B.h = readMem(ptr + 1);
}
void CPU6502::op_JSR()
{
    ++m_regs.pc.W;
    push(m_regs.pc.B.h);
    push(m_regs.pc.B.l);
    c6502_byte_t l = readMem(m_regs.pc.W - 1);
    c6502_byte_t h = readMem(m_regs.pc.W);
    m_regs.pc.B.l = l;
    m_regs.pc.B.h = h;
}

void CPU6502::op_LDA_IMM()
{
    m_regs.a = fetchImmOp();
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_LDA_ZP()
{
    m_regs.a = fetchZPOp();
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_LDA_ZPX()
{
    m_regs.a = fetchZPXOp();
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_LDA_ABS()
{
    m_regs.a = fetchABSOp();
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_LDA_ABX()
{
    m_regs.a = fetchABXOp();
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_LDA_ABY()
{
    m_regs.a = fetchABYOp();
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_LDA_INX()
{
    m_regs.a = fetchINXOp();
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_LDA_INY()
{
    m_regs.a = fetchINYOp();
    EVAL_N(m_regs.a);
    EVAL_Z(m_regs.a);
}
void CPU6502::op_LDX_IMM()
{
    m_regs.x = fetchImmOp();
    EVAL_N(m_regs.x);
    EVAL_Z(m_regs.x);
}
void CPU6502::op_LDX_ZP()
{
    m_regs.x = fetchZPOp();
    EVAL_N(m_regs.x);
    EVAL_Z(m_regs.x);
}
void CPU6502::op_LDX_ZPY()
{
    m_regs.x = fetchZPYOp();
    EVAL_N(m_regs.x);
    EVAL_Z(m_regs.x);
}
void CPU6502::op_LDX_ABS()
{
    m_regs.x = fetchABSOp();
    EVAL_N(m_regs.x);
    EVAL_Z(m_regs.x);
}
void CPU6502::op_LDX_ABY()
{
    m_regs.x = fetchABYOp();
    EVAL_N(m_regs.x);
    EVAL_Z(m_regs.x);
}
void CPU6502::op_LDY_IMM()
{
    m_regs.y = fetchImmOp();
    EVAL_N(m_regs.y);
    EVAL_Z(m_regs.y);
}
void CPU6502::op_LDY_ZP()
{
    m_regs.y = fetchZPOp();
    EVAL_N(m_regs.y);
    EVAL_Z(m_regs.y);
}
void CPU6502::op_LDY_ZPX()
{
    m_regs.y = fetchZPXOp();
    EVAL_N(m_regs.y);
    EVAL_Z(m_regs.y);
}
void CPU6502::op_LDY_ABS()
{
m_regs.y = fetchABSOp();
    EVAL_N(m_regs.y);
    EVAL_Z(m_regs.y);
}
void CPU6502::op_LDY_ABX()
{
    m_regs.y = fetchABXOp();
    EVAL_N(m_regs.y);
    EVAL_Z(m_regs.y);
}
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
void CPU6502::op_RTS()
{
    m_regs.pc.B.l = pop();
    m_regs.pc.B.h = pop();
    ++m_regs.pc.W;
}
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