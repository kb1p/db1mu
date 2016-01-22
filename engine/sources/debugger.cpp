#include "cpu6502.h"
#include "debugger.h"
#include <iostream>
#include <iomanip>
#include <string.h>
#include <DebugCommand.h>
#include "dbg_cmd.y.hpp"


int yyparse (DebugCommand* cmdParsed);
typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string (const char * yystr );
void yy_delete_buffer (YY_BUFFER_STATE b);

Debugger::Debugger(CPU6502 *cpu)
        : m_cpu(cpu), m_clock(*this)
{
}

void Debugger::Clock()
{
    InterruptIfNeed();
    m_cpu->Clock();
}

void Debugger::Start(long freq)
{
    Reset();
    m_clock.Start(freq);
}

void Debugger::Reset()
{
    m_cpu->reset();
    std::cout << " < S T A R T >\n";
    Interact();
}

void Debugger::Interact()
{
    char cmdLine[256];
    DebugCommand cmd;
    do {
        std::cout << "(db1mu-dbg)" << std::hex << std::setfill('0') << std::setw(4) << int(m_cpu->m_regs.pc.W) << "> ";
        std::cin.getline(cmdLine, sizeof(cmdLine));
        if (std::cin.eof()) {
            std::cout << "\n^D\n\n";
            exit(0);
        }
        cmd = ParseDebugCommand(cmdLine);
        switch (cmd.cmd) {
            case DebugCommand::CMD_PrintCPUState:
                PrintCPUState();
                break;
            case DebugCommand::CMD_Continue:
                break;
            case DebugCommand::CMD_PrintMemoryByte:
                PrintMem(cmd.args.mem_byte);
                break;
            case DebugCommand::CMD_PrintMemoryArray:
                PrintMem(cmd.args.mem_array.mem_ptr, cmd.args.mem_array.array_len);
                break;
            case DebugCommand::CMD_Break:
                SetBreak(cmd.args.break_addr);
                break;
            case DebugCommand::CMD_RST:
                Reset();
                break;
            case DebugCommand::CMD_Unknown:
                std::cout << "unknown DebugCommand " << cmdLine << "\n";
                break;
        }
    } while (cmd.cmd != DebugCommand::CMD_Continue);
}

void Debugger::InterruptIfNeed()
{
    bool needBreak = m_breaks.count(m_cpu->m_regs.pc.W) == 1; // todo: Check WATCHs
    if (needBreak)
        Interact();
}

DebugCommand Debugger::ParseDebugCommand(const char *cmdLine)
{
    DebugCommand cmd;

    YY_BUFFER_STATE bid = yy_scan_string(cmdLine);
    if (yyparse(&cmd))
        cmd.cmd = DebugCommand::CMD_Unknown;
    yy_delete_buffer(bid);
    return cmd;
}

void Debugger::PrintCPUState()
{
    std::cout << "| a| x| y| s| p|\t| pc |\t|c|z|i|d|b|v|n|\n";
    std::cout << std::hex
    << "|" << std::setfill('0') << std::setw(2) << int(m_cpu->m_regs.a)
    << "|" << std::setfill('0') << std::setw(2) << int(m_cpu->m_regs.x)
    << "|" << std::setfill('0') << std::setw(2) << int(m_cpu->m_regs.y)
    << "|" << std::setfill('0') << std::setw(2) << int(m_cpu->m_regs.s)
    << "|" << std::setfill('0') << std::setw(2) << int(m_cpu->m_regs.p.reg)
    << "|" << "\t"
    << "|" << std::setfill('0') << std::setw(4) << int(m_cpu->m_regs.pc.W)
    << "|" << "\t"
    << "|" << std::setfill('0') << std::setw(1) << int(m_cpu->m_regs.p.flags.c)
    << "|" << std::setfill('0') << std::setw(1) << int(m_cpu->m_regs.p.flags.z)
    << "|" << std::setfill('0') << std::setw(1) << int(m_cpu->m_regs.p.flags.i)
    << "|" << std::setfill('0') << std::setw(1) << int(m_cpu->m_regs.p.flags.d)
    << "|" << std::setfill('0') << std::setw(1) << int(m_cpu->m_regs.p.flags.b)
    << "|" << std::setfill('0') << std::setw(1) << int(m_cpu->m_regs.p.flags.v)
    << "|" << std::setfill('0') << std::setw(1) << int(m_cpu->m_regs.p.flags.n)
    << "|"
    << "\n";
    std::cout << "period = " << std::dec << m_cpu->m_period << "\n";

}

void Debugger::PrintMem(c6502_word_t ptr)
{
    std::cout << std::hex << "0x" << std::setfill('0') << std::setw(4) <<  ptr << ": " << c6502_word_t(m_cpu->readMem(ptr)) << "\n";
}
void Debugger::PrintMem(c6502_word_t ptr, c6502_word_t len)
{
    std::cout << std::hex << "0x" << ptr << ":";
    for (c6502_word_t i = 0; i < len; ++i) {
        if (i % 16 == 0)
            std::cout << "\n";
        else if (i % 8 == 0)
            std::cout << "   ";
        std::cout << std::setfill('0') << std::setw(2) << c6502_word_t(m_cpu->readMem(ptr + i)) << " ";
    }
    std::cout << "\n";
}

void Debugger::SetBreak(c6502_word_t ptr)
{
    std::cout << "New break at 0x" << std::setfill('0') << std::setw(4) <<  ptr << "\n";
    m_breaks.insert(ptr);
}