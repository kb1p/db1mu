/*
 * 6502 CPU emulation routines.
 *
 * This file contains only basic CPU routines; opcode implementations
 * are located in a separate module "opcodes.cpp".
 *
 * TODO: replace all these magic numbers with #defines at least.
 */

#include "cpu6502.h"
#include "debugger.h"
#include "Cartridge.h"
#include "PPU.h"
#include "log.h"
#include <stddef.h>
#include <cassert>

typedef unsigned int uint;

// TRACE shorthand for branching operations
#define TRACE_B(name, c) TRACE(name " cond=%s", (c) ? "true" : "false")

template <>
c6502_word_t CPU6502::fetchAddr<CPU6502::AM::ZP>() noexcept
{
    const c6502_word_t ea = readMem(m_regs.pc++);
    TRACE("Mode = ZP; addr = %X", ea);
    return ea;
}

template <>
c6502_word_t CPU6502::fetchAddr<CPU6502::AM::ZP_X>() noexcept
{
    const c6502_word_t addr = static_cast<c6502_word_t>(readMem(m_regs.pc++)) + m_regs.x;
    const auto ea = addr > 0xFFu ? 0 : addr;
    TRACE("Mode = ZP,X; addr = %X", ea);
    return ea;
}

template <>
c6502_word_t CPU6502::fetchAddr<CPU6502::AM::ZP_Y>() noexcept
{
    const c6502_word_t addr = static_cast<c6502_word_t>(readMem(m_regs.pc++)) + m_regs.y;
    const auto ea = addr > 0xFFu ? 0 : addr;
    TRACE("Mode = ZP,Y; addr = %X", ea);
    return ea;
}

template <>
c6502_word_t CPU6502::fetchAddr<CPU6502::AM::ABS>() noexcept
{
    const c6502_word_t al = readMem(m_regs.pc++),
                       ah = readMem(m_regs.pc++);
    const auto ea = al | (ah << 8);
    TRACE("Mode = ABS; addr = %X", ea);
    return ea;
}

template <>
c6502_word_t CPU6502::fetchAddr<CPU6502::AM::ABS_X>() noexcept
{
    const c6502_word_t al = readMem(m_regs.pc++),
                       ah = readMem(m_regs.pc++);

    // Page bound crossing check: if the lsb + index affects msb
    m_penalty = (al + m_regs.x > 0xFFu) ? 1 : 0;
    const auto ea = (al | (ah << 8)) + m_regs.x;
    TRACE("Mode = ABS, X; addr = %X", ea);

    return ea;
}

template <>
c6502_word_t CPU6502::fetchAddr<CPU6502::AM::ABS_Y>() noexcept
{
    const c6502_word_t al = readMem(m_regs.pc++),
                       ah = readMem(m_regs.pc++);

    m_penalty = (al + m_regs.y > 0xFFu) ? 1 : 0;

    const auto ea = (al | (ah << 8)) + m_regs.y;
    TRACE("Mode = ABS, Y; addr = %X", ea);

    return ea;
}

template <>
c6502_word_t CPU6502::fetchAddr<CPU6502::AM::IND_X>() noexcept
{
    const c6502_word_t baddr = (static_cast<c6502_word_t>(readMem(m_regs.pc++)) + m_regs.x) & 0xFFu,
                       laddr = readMem(baddr),
                       haddr = readMem((baddr + 1) & 0xFFu);
    const auto ea = laddr | (haddr << 8);
    TRACE("Mode = IND, X; addr = %X", ea);

    return ea;
}

template <>
c6502_word_t CPU6502::fetchAddr<CPU6502::AM::IND_Y>() noexcept
{
    const c6502_word_t baddr = readMem(m_regs.pc++),
                       laddr = readMem(baddr),
                       haddr = readMem((baddr + 1) & 0xFFu);

    m_penalty = (laddr + m_regs.y > 0xFFu) ? 1 : 0;

    const auto ea = (laddr | (haddr << 8)) + m_regs.y;
    TRACE("Mode = IND, Y; addr = %X", ea);

    return ea;
}

template <>
c6502_word_t CPU6502::fetchAddr<CPU6502::AM::IND>() noexcept
{
    auto al = readMem(m_regs.pc++),
         ah = readMem(m_regs.pc++);
    const c6502_word_t opaddr = combine(al, ah);
    al = readMem(opaddr);
    ah = readMem(opaddr + 1);

    const auto ea = combine(al, ah);
    TRACE("Mode = IND; addr = %X", ea);

    return ea;
}

template <>
c6502_byte_t CPU6502::fetchOperand<CPU6502::AM::IMM>() noexcept
{
    const auto eo = readMem(m_regs.pc++);
    TRACE("Mode = IMM; op. value = %X", eo);
    return eo;
}

template <>
c6502_byte_t CPU6502::fetchOperand<CPU6502::AM::ACC>() noexcept
{
    TRACE("Mode = ACC; op. value = %X", m_regs.a);
    return m_regs.a;
}

// 6502 commands
#define CMD_DEF(name) \
template <CPU6502::AM MODE> \
void CPU6502::cmd_##name() noexcept

#define CMD_DEF_SPEC(name, mode) \
template <> \
void CPU6502::cmd_##name<CPU6502::AM::mode>() noexcept

CMD_DEF(ADC)
{
    TRACE("ADC");
    const c6502_word_t op = fetchOperand<MODE>();
    const c6502_word_t r = static_cast<c6502_word_t>(m_regs.a) + getFlag<Flag::C>() + op;

    eval_C(r);
    eval_Z(r);
    eval_N(r);
    setFlag<Flag::V>(((m_regs.a ^ op) & 0x80u) == 0u && ((m_regs.a ^ r) & 0x80u) != 0u ? 1u : 0u);

    m_regs.a = lo_byte(r);
}

CMD_DEF(AND)
{
    TRACE("AND");
    const auto op = fetchOperand<MODE>();

    m_regs.a &= op;

    eval_Z(m_regs.a);
    eval_N(m_regs.a);
}

CMD_DEF(ASL)
{
    static_assert(MODE != AM::ACC && MODE != AM::IMM, "Illegal addressing mode for ASL instruction");
    TRACE("ASL");
    const auto addr = fetchAddr<MODE>();
    auto op = readMem(addr);

    setFlag<Flag::C>((0x80u & op) >> 7);
    op <<= 1;

    eval_N(op);
    eval_Z(op);

    writeMem(addr, op);
}

CMD_DEF_SPEC(ASL, ACC)
{
    TRACE("ASL A (A = %X)", m_regs.a);
    setFlag<Flag::C>((0x80u & m_regs.a) >> 7);
    m_regs.a <<= 1;

    eval_N(m_regs.a);
    eval_Z(m_regs.a);
}

CMD_DEF(BCC)
{
    TRACE("BCC");
    branchIf<Flag::C, false>();
}

CMD_DEF(BCS)
{
    TRACE("BCS");
    branchIf<Flag::C, true>();
}

CMD_DEF(BEQ)
{
    TRACE("BEQ");
    branchIf<Flag::Z, true>();
}

CMD_DEF(BIT)
{
    TRACE("BIT");
    const auto op = fetchOperand<MODE>();
    const auto r = m_regs.a & op;

    eval_Z(r);
    eval_N(r);
    setFlag<Flag::V>((r >> 6) & 0x1u);
}

CMD_DEF(BMI)
{
    TRACE("BMI");
    branchIf<Flag::N, true>();
}

CMD_DEF(BNE)
{
    TRACE("BNE");
    branchIf<Flag::Z, false>();
}

CMD_DEF(BPL)
{
    TRACE("BPL");
    branchIf<Flag::N, false>();
}

CMD_DEF(BRK)
{
    push(hi_byte(m_regs.pc));
    push(lo_byte(m_regs.pc));
    setFlag<Flag::B>(1);
    push(m_regs.p);
    setFlag<Flag::I>(1);

    const auto l = readMem(0xFFFE),
               h = readMem(0xFFFF);
    const auto ea = combine(l, h);

    TRACE("BRK to %X", ea);

    m_regs.pc = ea;
}

CMD_DEF(BVC)
{
    TRACE("BVC");
    branchIf<Flag::V, false>();
}

CMD_DEF(BVS)
{
    TRACE("BVS");
    branchIf<Flag::V, true>();
}

CMD_DEF(CLC)
{
    TRACE("CLC");
    setFlag<Flag::C>(0);
}

CMD_DEF(CLD)
{
    TRACE("CLD");
    setFlag<Flag::D>(0);
}

CMD_DEF(CLI)
{
    TRACE("CLI");
    setFlag<Flag::I>(0);
}

CMD_DEF(CLV)
{
    TRACE("CLV");
    setFlag<Flag::V>(0);
}

CMD_DEF(CMP)
{
    TRACE("CMP");
    const auto op = fetchOperand<MODE>();

    uint r = m_regs.a;
    r -= op;

    setFlag<Flag::C>(r < 0x100 ? 1 : 0);
    eval_Z(static_cast<c6502_byte_t>(r & 0xFF));
    eval_N(static_cast<c6502_byte_t>(r & 0xFF));
}

CMD_DEF(CPX)
{
    TRACE("CPX");
    const auto op = fetchOperand<MODE>();

    uint r = m_regs.x;
    r -= op;

    setFlag<Flag::C>(r < 0x100 ? 1 : 0);
    eval_Z(static_cast<c6502_byte_t>(r & 0xFF));
    eval_N(static_cast<c6502_byte_t>(r & 0xFF));
}

CMD_DEF(CPY)
{
    TRACE("CPY");
    const auto op = fetchOperand<MODE>();

    uint r = m_regs.y;
    r -= op;

    setFlag<Flag::C>(r < 0x100 ? 1 : 0);
    eval_Z(static_cast<c6502_byte_t>(r & 0xFF));
    eval_N(static_cast<c6502_byte_t>(r & 0xFF));
}

CMD_DEF(DEC)
{
    TRACE("DEC");
    const auto addr = fetchAddr<MODE>();
    const auto op = readMem(addr);
    const auto r = op - 1;
    eval_N(r);
    eval_Z(r);
    writeMem(addr, r);
}

CMD_DEF(DEX)
{
    TRACE("DEX (X = %X)", m_regs.x);
    m_regs.x--;
    eval_N(m_regs.x);
    eval_Z(m_regs.x);
}

CMD_DEF(DEY)
{
    TRACE("DEY (Y = %X)", m_regs.y);
    m_regs.y--;
    eval_N(m_regs.y);
    eval_Z(m_regs.y);
}

CMD_DEF(EOR)
{
    TRACE("EOR");
    const auto op = fetchOperand<MODE>();
    m_regs.a ^= op;
    eval_N(m_regs.a);
    eval_Z(m_regs.a);
}

CMD_DEF(INC)
{
    TRACE("INC");
    const auto addr = fetchAddr<MODE>();
    const auto op = readMem(addr);
    const auto r = op + 1;
    eval_N(r);
    eval_Z(r);
    writeMem(addr, r);
}

CMD_DEF(INX)
{
    TRACE("INX (X = %X)", m_regs.x);
    m_regs.x++;
    eval_N(m_regs.x);
    eval_Z(m_regs.x);
}

CMD_DEF(INY)
{
    TRACE("INY (Y = %X)", m_regs.y);
    m_regs.y++;
    eval_N(m_regs.y);
    eval_Z(m_regs.y);
}

CMD_DEF(JMP)
{
    TRACE("JMP");
    const auto ea = fetchAddr<MODE>();
    m_regs.pc = ea;
}

CMD_DEF(JSR)
{
    TRACE("JSR");
    const auto where = fetchAddr<MODE>();
    push(hi_byte(m_regs.pc));
    push(lo_byte(m_regs.pc));
    m_regs.pc = where;
}

CMD_DEF(LDA)
{
    TRACE("LDA");
    const auto op = fetchOperand<MODE>();
    eval_N(op);
    eval_Z(op);
    m_regs.a = op;
}

CMD_DEF(LDX)
{
    TRACE("LDX");
    const auto op = fetchOperand<MODE>();
    eval_N(op);
    eval_Z(op);
    m_regs.x = op;
}

CMD_DEF(LDY)
{
    TRACE("LDY");
    const auto op = fetchOperand<MODE>();
    eval_N(op);
    eval_Z(op);
    m_regs.y = op;
}

CMD_DEF(LSR)
{
    static_assert(MODE != AM::ACC && MODE != AM::IMM, "Illegal addressing mode for LSR instruction");
    TRACE("LSR");
    const auto addr = fetchAddr<MODE>();
    auto op = readMem(addr);
    setFlag<Flag::C>(op & 1u);
    op >>= 1;
    eval_N(op);
    eval_Z(op);
    writeMem(addr, op);
}

CMD_DEF_SPEC(LSR, ACC)
{
    TRACE("LSR A (A = %X)", m_regs.a);
    setFlag<Flag::C>(m_regs.a & 1u);
    m_regs.a >>= 1;
    eval_N(m_regs.a);
    eval_Z(m_regs.a);
}

CMD_DEF(NOP)
{
    TRACE("NOP");
}

CMD_DEF(ORA)
{
    TRACE("ORA");
    const auto op = fetchOperand<MODE>();
    m_regs.a |= op;
    eval_N(m_regs.a);
    eval_Z(m_regs.a);
}

CMD_DEF(PHA)
{
    TRACE("PHA");
    push(m_regs.a);
}

CMD_DEF(PHP)
{
    TRACE("PHP");
    push(m_regs.p);
}

CMD_DEF(PLA)
{
    TRACE("PLA");
    m_regs.a = pop();
    eval_N(m_regs.a);
    eval_Z(m_regs.a);
}

CMD_DEF(PLP)
{
    TRACE("PLP");
    m_regs.p = pop();
}

CMD_DEF(ROL)
{
    static_assert(MODE != AM::ACC && MODE != AM::IMM, "Illegal addressing mode for ROL instruction");
    TRACE("ROL");
    const auto addr = fetchAddr<MODE>();
    c6502_word_t op = readMem(addr);
    op <<= 1;
    op |= getFlag<Flag::C>();
    eval_C(op);
    const auto bop = static_cast<c6502_byte_t>(op & 0xFFu);
    eval_N(bop);
    eval_Z(bop);
    writeMem(addr, bop);
}

CMD_DEF_SPEC(ROL, ACC)
{
    TRACE("ROL A (A = %X)", m_regs.a);
    c6502_word_t op = m_regs.a;
    op <<= 1;
    op |= getFlag<Flag::C>();
    eval_C(op);
    m_regs.a = static_cast<c6502_byte_t>(op & 0xFFu);
    eval_N(m_regs.a);
    eval_Z(m_regs.a);
}

CMD_DEF(ROR)
{
    static_assert(MODE != AM::ACC && MODE != AM::IMM, "Illegal addressing mode for ROR instruction");
    TRACE("ROR");
    const auto addr = fetchAddr<MODE>();
    c6502_word_t op = readMem(addr);
    if (getFlag<Flag::C>() != 0)
        op |= 0x100u;
    setFlag<Flag::C>(op & 1u);
    op >>= 1;
    const auto bop = static_cast<c6502_byte_t>(op & 0xFFu);
    eval_N(bop);
    eval_Z(bop);
    writeMem(addr, bop);
}

CMD_DEF_SPEC(ROR, ACC)
{
    TRACE("ROR A (A = %X)", m_regs.a);
    c6502_word_t op = m_regs.a;
    if (getFlag<Flag::C>() != 0)
        op |= 0x100u;
    setFlag<Flag::C>(op & 1u);
    op >>= 1;
    m_regs.a = static_cast<c6502_byte_t>(op & 0xFFu);
    eval_N(m_regs.a);
    eval_Z(m_regs.a);
}

CMD_DEF(RTI)
{
    TRACE("RTI");
    m_regs.p = pop();
    const auto ral = pop(),
               rah = pop();
    m_regs.pc = combine(ral, rah);
}

CMD_DEF(RTS)
{
    TRACE("RTS");
    const auto ral = pop(),
               rah = pop();
    m_regs.pc = combine(ral, rah);
}

CMD_DEF(SBC)
{
    TRACE("SBC");
    const uint op = fetchOperand<MODE>(),
               borrow = getFlag<Flag::C>() ^ 1u;
    const uint r = static_cast<int>(m_regs.a) - op - borrow;
    const auto br = static_cast<c6502_byte_t>(r & 0xFF);
    eval_N(br);
    eval_Z(br);
    setFlag<Flag::V>(((m_regs.a ^ r) & 0x80) != 0 && ((m_regs.a ^ op) & 0x80) != 0 ? 1 : 0);
    setFlag<Flag::C>(r < 0x100 ? 1 : 0);

    m_regs.a = br;
}

CMD_DEF(SEC)
{
    TRACE("SEC");
    setFlag<Flag::C>(1u);
}

CMD_DEF(SED)
{
    TRACE("SED");
    setFlag<Flag::D>(1u);
}

CMD_DEF(SEI)
{
    TRACE("SEI");
    setFlag<Flag::I>(1u);
}

CMD_DEF(STA)
{
    TRACE("STA");
    writeMem(fetchAddr<MODE>(), m_regs.a);
}

CMD_DEF(STX)
{
    TRACE("STX");
    writeMem(fetchAddr<MODE>(), m_regs.x);
}

CMD_DEF(STY)
{
    TRACE("STY");
    writeMem(fetchAddr<MODE>(), m_regs.y);
}

CMD_DEF(TAX)
{
    TRACE("TAX");
    eval_N(m_regs.a);
    eval_Z(m_regs.a);
    m_regs.x = m_regs.a;
}

CMD_DEF(TAY)
{
    TRACE("TAY");
    eval_N(m_regs.a);
    eval_Z(m_regs.a);
    m_regs.y = m_regs.a;
}

CMD_DEF(TSX)
{
    TRACE("TSX");
    eval_N(m_regs.s);
    eval_Z(m_regs.s);
    m_regs.x = m_regs.s;
}

CMD_DEF(TXA)
{
    TRACE("TXA");
    eval_N(m_regs.x);
    eval_Z(m_regs.x);
    m_regs.a = m_regs.x;
}

CMD_DEF(TXS)
{
    TRACE("TXS");
    m_regs.s = m_regs.x;
}

CMD_DEF(TYA)
{
    TRACE("TYA");
    eval_N(m_regs.y);
    eval_Z(m_regs.y);
    m_regs.a = m_regs.y;
}

#undef CMD_DEF

std::array<CPU6502::OpData, CPU6502::OPCODE_COUNT> CPU6502::s_opHandlers;

void CPU6502::initOpHandlers() noexcept
{
    s_opHandlers.fill(std::make_tuple(nullptr, -1, false));

#define BIND_OP(name, am, opcode, tacts, penalty) \
    { assert(std::get<1>(s_opHandlers[(opcode)]) == -1); \
    s_opHandlers[(opcode)] = std::make_tuple(&CPU6502::cmd_##name<AM::am>, (tacts), (penalty)); }

    // ADC
    BIND_OP(ADC, IMM,   0x69, 2, false)
    BIND_OP(ADC, ZP,    0x65, 3, false)
    BIND_OP(ADC, ZP_X,  0x75, 4, false)
    BIND_OP(ADC, ABS,   0x6D, 4, false)
    BIND_OP(ADC, ABS_X, 0x7D, 4, true)
    BIND_OP(ADC, ABS_Y, 0x79, 4, true)
    BIND_OP(ADC, IND_X, 0x61, 6, false)
    BIND_OP(ADC, IND_Y, 0x71, 5, true)
    // AND
    BIND_OP(AND, IMM,   0x29, 2, false)
    BIND_OP(AND, ZP,    0x25, 3, false)
    BIND_OP(AND, ZP_X,  0x35, 4, false)
    BIND_OP(AND, ABS,   0x2D, 4, false)
    BIND_OP(AND, ABS_X, 0x3D, 4, true)
    BIND_OP(AND, ABS_Y, 0x39, 4, true)
    BIND_OP(AND, IND_X, 0x21, 6, false)
    BIND_OP(AND, IND_Y, 0x31, 5, true)
    // ASL
    BIND_OP(ASL, ACC,   0x0A, 2, false)
    BIND_OP(ASL, ZP,    0x06, 5, false)
    BIND_OP(ASL, ZP_X,  0x16, 6, false)
    BIND_OP(ASL, ABS,   0x0E, 6, false)
    BIND_OP(ASL, ABS_X, 0x1E, 7, false)
    // Branches
    BIND_OP(BCC, DEF,   0x90, 2, true)
    BIND_OP(BCS, DEF,   0xB0, 2, true)
    BIND_OP(BEQ, DEF,   0xF0, 2, true)
    BIND_OP(BMI, DEF,   0x30, 2, true)
    BIND_OP(BNE, DEF,   0xD0, 2, true)
    BIND_OP(BPL, DEF,   0x10, 2, true)
    BIND_OP(BVC, DEF,   0x50, 2, true)
    BIND_OP(BVS, DEF,   0x70, 2, true)
    // BIT
    BIND_OP(BIT, ZP,    0x24, 3, false)
    BIND_OP(BIT, ABS,   0x2C, 4, false)
    // BRK
    BIND_OP(BRK, DEF,   0x00, 7, false)
    // CLC
    BIND_OP(CLC, DEF,   0x18, 2, false)
    // CLD
    BIND_OP(CLD, DEF,   0xD8, 2, false)
    // CLI
    BIND_OP(CLI, DEF,   0x58, 2, false)
    // CLV
    BIND_OP(CLV, DEF,   0xB8, 2, false)
    // CMP
    BIND_OP(CMP, IMM,   0xC9, 2, false)
    BIND_OP(CMP, ZP,    0xC5, 3, false)
    BIND_OP(CMP, ZP_X,  0xD5, 4, false)
    BIND_OP(CMP, ABS,   0xCD, 4, false)
    BIND_OP(CMP, ABS_X, 0xDD, 4, true)
    BIND_OP(CMP, ABS_Y, 0xD9, 4, true)
    BIND_OP(CMP, IND_X, 0xC1, 6, false)
    BIND_OP(CMP, IND_Y, 0xD1, 5, true)
    // CPX
    BIND_OP(CPX, IMM,   0xE0, 2, false)
    BIND_OP(CPX, ZP,    0xE4, 3, false)
    BIND_OP(CPX, ABS,   0xEC, 4, false)
    // CPY
    BIND_OP(CPY, IMM,   0xC0, 2, false)
    BIND_OP(CPY, ZP,    0xC4, 3, false)
    BIND_OP(CPY, ABS,   0xCC, 4, false)
    // DEC
    BIND_OP(DEC, ZP,    0xC6, 5, false)
    BIND_OP(DEC, ZP_X,  0xD6, 6, false)
    BIND_OP(DEC, ABS,   0xCE, 6, false)
    BIND_OP(DEC, ABS_X, 0xDE, 7, false)
    // DEX
    BIND_OP(DEX, DEF,   0xCA, 2, false)
    // DEY
    BIND_OP(DEY, DEF,   0x88, 2, false)
    // EOR
    BIND_OP(EOR, IMM,   0x49, 2, false)
    BIND_OP(EOR, ZP,    0x45, 3, false)
    BIND_OP(EOR, ZP_X,  0x55, 4, false)
    BIND_OP(EOR, ABS,   0x4D, 4, false)
    BIND_OP(EOR, ABS_X, 0x5D, 4, true)
    BIND_OP(EOR, ABS_Y, 0x59, 4, true)
    BIND_OP(EOR, IND_X, 0x41, 6, false)
    BIND_OP(EOR, IND_Y, 0x51, 5, true)
    // INC
    BIND_OP(INC, ZP,    0xE6, 5, false)
    BIND_OP(INC, ZP_X,  0xF6, 6, false)
    BIND_OP(INC, ABS,   0xEE, 6, false)
    BIND_OP(INC, ABS_X, 0xFE, 7, false)
    // INX
    BIND_OP(INX, DEF,   0xE8, 2, false)
    // INY
    BIND_OP(INY, DEF,   0xC8, 2, false)
    // JMP
    BIND_OP(JMP, ABS,   0x4C, 3, false)
    BIND_OP(JMP, IND,   0x6C, 5, false)
    // JSR
    BIND_OP(JSR, ABS,   0x20, 6, false)
    // LDA
    BIND_OP(LDA, IMM,   0xA9, 2, false)
    BIND_OP(LDA, ZP,    0xA5, 3, false)
    BIND_OP(LDA, ZP_X,  0xB5, 4, false)
    BIND_OP(LDA, ABS,   0xAD, 4, false)
    BIND_OP(LDA, ABS_X, 0xBD, 4, true)
    BIND_OP(LDA, ABS_Y, 0xB9, 4, true)
    BIND_OP(LDA, IND_X, 0xA1, 6, false)
    BIND_OP(LDA, IND_Y, 0xB1, 5, true)
    // LDX
    BIND_OP(LDX, IMM,   0xA2, 2, false)
    BIND_OP(LDX, ZP,    0xA6, 3, false)
    BIND_OP(LDX, ZP_Y,  0xB6, 4, false)
    BIND_OP(LDX, ABS,   0xAE, 4, false)
    BIND_OP(LDX, ABS_Y, 0xBE, 4, true)
    // LDY
    BIND_OP(LDY, IMM,   0xA0, 2, false)
    BIND_OP(LDY, ZP,    0xA4, 3, false)
    BIND_OP(LDY, ZP_X,  0xB4, 4, false)
    BIND_OP(LDY, ABS,   0xAC, 4, false)
    BIND_OP(LDY, ABS_X, 0xBC, 4, true)
    // LSR
    BIND_OP(LSR, ACC,   0x4A, 2, false)
    BIND_OP(LSR, ZP,    0x46, 5, false)
    BIND_OP(LSR, ZP_X,  0x56, 6, false)
    BIND_OP(LSR, ABS,   0x4E, 6, false)
    BIND_OP(LSR, ABS_X, 0x5E, 7, false)
    // NOP
    BIND_OP(NOP, DEF,   0xEA, 2, false)
    // ORA
    BIND_OP(ORA, IMM,   0x09, 2, false)
    BIND_OP(ORA, ZP,    0x05, 3, false)
    BIND_OP(ORA, ZP_X,  0x15, 4, false)
    BIND_OP(ORA, ABS,   0x0D, 4, false)
    BIND_OP(ORA, ABS_X, 0x1D, 4, true)
    BIND_OP(ORA, ABS_Y, 0x19, 4, true)
    BIND_OP(ORA, IND_X, 0x01, 6, false)
    BIND_OP(ORA, IND_Y, 0x11, 5, true)
    // PHA
    BIND_OP(PHA, DEF,   0x48, 3, false)
    // PHP
    BIND_OP(PHP, DEF,   0x08, 3, false)
    // PLA
    BIND_OP(PLA, DEF,   0x68, 4, false)
    // PLP
    BIND_OP(PLP, DEF,   0x28, 4, false)
    // ROL
    BIND_OP(ROL, ACC,   0x2A, 2, false)
    BIND_OP(ROL, ZP,    0x26, 5, false)
    BIND_OP(ROL, ZP_X,  0x36, 6, false)
    BIND_OP(ROL, ABS,   0x2E, 6, false)
    BIND_OP(ROL, ABS_X, 0x3E, 7, false)
    // ROR
    BIND_OP(ROR, ACC,   0x6A, 2, false)
    BIND_OP(ROR, ZP,    0x66, 5, false)
    BIND_OP(ROR, ZP_X,  0x76, 6, false)
    BIND_OP(ROR, ABS,   0x6E, 6, false)
    BIND_OP(ROR, ABS_X, 0x7E, 7, false)
    // RTI
    BIND_OP(RTI, DEF,   0x40, 6, false)
    // RTS
    BIND_OP(RTS, DEF,   0x60, 6, false)
    // SBC
    BIND_OP(SBC, IMM,   0xE9, 2, false)
    BIND_OP(SBC, ZP,    0xE5, 3, false)
    BIND_OP(SBC, ZP_X,  0xF5, 4, false)
    BIND_OP(SBC, ABS,   0xED, 4, false)
    BIND_OP(SBC, ABS_X, 0xFD, 4, true)
    BIND_OP(SBC, ABS_Y, 0xF9, 4, true)
    BIND_OP(SBC, IND_X, 0xE1, 6, false)
    BIND_OP(SBC, IND_Y, 0xF1, 5, true)
    // SEC
    BIND_OP(SEC, DEF,   0x38, 2, false)
    // SED
    BIND_OP(SED, DEF,   0xF8, 2, false)
    // SEI
    BIND_OP(SEI, DEF,   0x78, 2, false)
    // STA
    BIND_OP(STA, ZP,    0x85, 3, false)
    BIND_OP(STA, ZP_X,  0x95, 4, false)
    BIND_OP(STA, ABS,   0x8D, 4, false)
    BIND_OP(STA, ABS_X, 0x9D, 5, false)
    BIND_OP(STA, ABS_Y, 0x99, 5, false)
    BIND_OP(STA, IND_X, 0x81, 6, false)
    BIND_OP(STA, IND_Y, 0x91, 6, false)
    // STX
    BIND_OP(STX, ZP,    0x86, 3, false)
    BIND_OP(STX, ZP_Y,  0x96, 4, false)
    BIND_OP(STX, ABS,   0x8E, 4, false)
    // STY
    BIND_OP(STY, ZP,    0x84, 3, false)
    BIND_OP(STY, ZP_X,  0x94, 4, false)
    BIND_OP(STY, ABS,   0x8C, 4, false)
    // TAX
    BIND_OP(TAX, DEF,   0xAA, 2, false)
    // TAY
    BIND_OP(TAY, DEF,   0xA8, 2, false)
    // TSX
    BIND_OP(TSX, DEF,   0xBA, 2, false)
    // TXA
    BIND_OP(TXA, DEF,   0x8A, 2, false)
    // TXS
    BIND_OP(TXS, DEF,   0x9A, 2, false)
    // TYA
    BIND_OP(TYA, DEF,   0x98, 2, false)

#undef BIND_OP
}

/*** CPU class implementation ***/
CPU6502::CPU6502(Bus &bus)
    : m_state(STATE_HALTED)
    , m_period(0)
    , m_bus { bus }
{
    // Static initializer
    static bool staticInitComplete = false;

    if (!staticInitComplete)
    {
        initOpHandlers();
        staticInitComplete = true;
    }
}

void CPU6502::reset()
{
    m_regs.a = m_regs.x = m_regs.y = 0;
    m_regs.p = 0x22;
    m_regs.s = 0xFF;
    const auto pcl = readMem(0xFFFC),
               pch = readMem(0xFFFD);
    m_regs.pc = combine(pcl, pch);

    m_period = 0;
    m_state = STATE_RUN;
}

// Handle maskable interrupt
void CPU6502::IRQ()
{
    if (getFlag<Flag::I>() == 0)
    {
        Log::v("IRQ");

        // Like BRK opcode, but without B flag
        push(hi_byte(m_regs.pc));
        push(lo_byte(m_regs.pc));
        push(m_regs.p);
        setFlag<Flag::I>(1);

        const auto pcl = readMem(0xFFFE),
                   pch = readMem(0xFFFF);
        m_regs.pc = combine(pcl, pch);

        m_period -= 7;
    }
}

// Handle non-maskable interrupt
void CPU6502::NMI()
{
    Log::v("NMI");
    push(hi_byte(m_regs.pc));
    push(lo_byte(m_regs.pc));
    push(m_regs.p);

    const auto pcl = readMem(0xFFFA),
               pch = readMem(0xFFFB);
    m_regs.pc = combine(pcl, pch);

    m_period -= 7;
}

void CPU6502::runFrame() noexcept
{
    m_period += m_bus.getMode() == OutputMode::PAL ? 35469 : 29830;
    while (m_period > 0)
    {
        switch (m_state)
        {
            case STATE_RUN:
                m_period -= step();
                break;
            case STATE_ERROR:
                Log::e("Unexpected CPU state (%d)", m_state);
            case STATE_HALTED:
                return;
        }
    }
}

int CPU6502::step()
{
    const auto opcode = advance();

    OpHandler oph;
    int tacts;
    bool usePenalty;
    std::tie(oph, tacts, usePenalty) = s_opHandlers[opcode];
    if (tacts > 0)
    {
        m_penalty = 0;
        (this->*oph)();
        return tacts + (usePenalty ? m_penalty : 0);
    }
    else
    {
        m_state = STATE_ERROR;

        Log::e("Bad opcode %X", opcode);
        assert(false && "Bad opcode");
        return 0;
    }
}
