#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <atomic>

class CPU6502;

class Debugger
{
public:
    Debugger(CPU6502* cpu);
    Debugger();
    void Attach(CPU6502* cpu);
    void Detach();
    void operator()();
private:

    struct Command {
        enum CMD {
            CMD_PrintCPUState,
            CMD_Continue,
            CMD_Unknown
        };
        CMD cmd;
    };

    Command ParseCommand(const char* cmd);
    void PrintCPUState();
    CPU6502* m_cpu;
    std::atomic_bool m_isAttached;

};

#endif // _DEBUGGER_H
