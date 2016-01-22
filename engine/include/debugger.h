#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <atomic>
#include "Clock.h"
#include "DebugCommand.h"
#include <set>


class CPU6502;

class Debugger
{
public:
    Debugger(CPU6502* cpu);
    void Clock();
    void Start(long freq);
private:

    void Reset();
    void InterruptIfNeed();
    DebugCommand ParseDebugCommand(const char* cmd);
    void PrintCPUState();
    void PrintMem(c6502_word_t ptr);
    void PrintMem(c6502_word_t ptr, c6502_word_t len);
    void SetBreak(c6502_word_t ptr);
    void Interact();
    CPU6502* m_cpu;
    class Clock<Debugger> m_clock;

    std::set<c6502_word_t> m_breaks;
};

#endif // _DEBUGGER_H
