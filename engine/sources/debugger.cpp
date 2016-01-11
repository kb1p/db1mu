#include "cpu6502.h"
#include "debugger.h"
#include <iostream>
#include <iomanip>
#include <string.h>

Debugger::Debugger(CPU6502* cpu)
: m_cpu(cpu)
, m_isAttached(true)
{
    m_cpu->m_dbg = this;
}

Debugger::Debugger()
: m_isAttached(false)
{ }

void Debugger::Attach(CPU6502* cpu)
{
    m_cpu = cpu;
    m_isAttached = true;
    m_cpu->m_dbg = this;
}
void Debugger::Detach()
{
    m_isAttached = false;
}

void Debugger::operator()()
{
    if (!m_isAttached)
        return;
    char cmdLine[256];
    Command command;
    do {
        std::cout << "(db1mu-dbg)> ";
        std::cin >> cmdLine;
        if (std::cin.eof()) {
            std::cout << "\n^D\n\n";
            exit(0);
        }
        command = ParseCommand(cmdLine);
        switch (command.cmd) {
            case Command::CMD_PrintCPUState:
                PrintCPUState();
                break;
            case Command::CMD_Unknown:
                std::cout << "unknown command " << cmdLine << "\n";
                break;
            case Command::CMD_Continue:
                break;
        }
    } while (command.cmd != Command::CMD_Continue);
}

Debugger::Command Debugger::ParseCommand(const char *cmdLine)
{
    Command cmd;
    if (strcmp(cmdLine, "pcpu") == 0) {
        cmd.cmd = Command::CMD_PrintCPUState;
    } else if (strcmp(cmdLine, "con") == 0) {
        cmd.cmd = Command::CMD_Continue;
    } else {
        cmd.cmd = Command::CMD_Unknown;
    }
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