#ifndef CLOCK_H
#define	CLOCK_H

#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>
#include <iomanip>
#include <atomic>

template <class PU>
class Clock
{
public:

    Clock(PU& pu)
    : m_pu(pu)
    , m_shouldContinue(false) { }

    void Start(long freq)
    {
        if (m_shouldContinue) {
            assert(0 && "clock already started");
            return;
        }
        m_shouldContinue = true;
        m_thread = std::move(std::thread(&Clock::InternalStart, this, freq));
    }

    void Stop()
    {
        m_shouldContinue = false;
        m_thread.join();
    }

private:
    typedef std::chrono::high_resolution_clock ClockInternal;

    void InternalStart(long freq)
    {
        auto next = ClockInternal::now();
        ClockInternal::duration period(ClockInternal::period::den / freq);
        while (m_shouldContinue) {
            next += period;
            std::this_thread::sleep_until(next);
            m_pu.Clock();
        }
    }

    PU& m_pu;
    std::thread m_thread;
    std::atomic_bool m_shouldContinue;
};

#define PAL_FREQ       1773447
#define NTSC_FREQ      1789772
#define PAL_PPU_FREQ   50
#define NTSC_PPU_FREQ  60

#endif	/* CLOCK_H */

