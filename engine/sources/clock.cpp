#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>
#include <iomanip>
#include <atomic>

double time_since_epoch()
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::duration<double> >(now).count();
}

class SampleCPU
{
public:

    SampleCPU()
    : m_tsc(0) { }

    void Clock()
    {
        ++m_tsc;
    }

    u_int64_t TSC() const
    {
        return m_tsc.load();
    }
private:
    std::atomic_uint_fast64_t m_tsc;

};

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

int main()
{
    {
        SampleCPU cpu;
        Clock<SampleCPU> clock(cpu);
        const int seconds = 7;
        clock.Start(PAL_FREQ);
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        clock.Stop();
        std::cout << "PAL CPU frequency:  " << cpu.TSC() / seconds << "  -- should be " << PAL_FREQ << "\n";
    }
    {
        SampleCPU cpu;
        Clock<SampleCPU> clock(cpu);
        const int seconds = 7;
        clock.Start(NTSC_FREQ);
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        clock.Stop();
        std::cout << "NTSC CPU frequency:  " << cpu.TSC() / seconds << "  -- should be " << NTSC_FREQ << "\n";
    }
    {
        SampleCPU cpu;
        Clock<SampleCPU> clock(cpu);
        const int seconds = 7;
        clock.Start(PAL_PPU_FREQ);
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        clock.Stop();
        std::cout << "PAL PPU frequency:  " << cpu.TSC() / seconds << "  -- should be " << PAL_PPU_FREQ << "\n";
    }
    {
        SampleCPU cpu;
        Clock<SampleCPU> clock(cpu);
        const int seconds = 7;
        clock.Start(NTSC_PPU_FREQ);
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        clock.Stop();
        std::cout << "NTSC PPU frequency:  " << cpu.TSC() / seconds << "  -- should be " << NTSC_PPU_FREQ << "\n";
    }
}
