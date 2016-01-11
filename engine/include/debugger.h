#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <atomic>
#include "Clock.h"


class CPU6502;

class Debugger
{
public:
    Debugger(CPU6502* cpu);
    void Clock();
    void Start(long freq);
private:

    struct Command {
        enum CMD {
            CMD_PrintCPUState,
            CMD_Continue,
            CMD_Unknown
        };
        CMD cmd;
    };

    void Reset();
    void InterruptIfNeed();
    Command ParseCommand(const char* cmd);
    void PrintCPUState();
    CPU6502* m_cpu;
    class Clock<Debugger> m_clock;
};

#endif // _DEBUGGER_H
