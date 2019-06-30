#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <atomic>
#include "Clock.h"
#include "DebugCommand.h"
#include "bus.h"
#include <set>

class Debugger
{
public:
    Debugger(Bus &bus);
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

    Bus &m_bus;
    class Clock<Debugger> m_clock;

    std::set<c6502_word_t> m_breaks;
    bool m_stepBreak = false;
};

#endif // _DEBUGGER_H
