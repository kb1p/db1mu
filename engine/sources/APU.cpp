#include "APU.h"
#include "log.h"
#include "bus.h"

#include <cassert>

void Envelope::clock() noexcept
{
    if (m_startFlag)
    {
        m_startFlag = false;
        m_dlc = 15u;
        m_divCnt = m_divPeriod;
    }
    else if (m_divCnt-- == 0)
    {
        m_divCnt = m_divPeriod;
        if (m_dlc-- == 0)
        {
            if (m_loop)
                m_dlc = 15u;
        }
    }
}

const uint PulseChannel::SEQUENCER[4][8] = {
    { 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 0, 0, 1, 1, 1, 1, 1 }
};

void PulseChannel::clockTimer() noexcept
{
    if (m_timer == 0u)
    {
        m_timer = m_timerSetup;

        // Trigger sequencer switch
        m_seqIndex = (m_seqIndex + 1) % 8;
    }
    else
        m_timer--;
}

void PulseChannel::clockLengthCounterSweep() noexcept
{
}

uint PulseChannel::sample() noexcept
{
    uint rv = 0u;

    // TODO: sweep
    if (m_timerSetup >= 8u &&
        m_lc > 0u &&
        SEQUENCER[m_dutySetup][m_seqIndex] > 0)
        rv = m_envelope.volume();

    return rv;
}

c6502_byte_t APU::readRegister(c6502_word_t reg)
{
    c6502_byte_t rv = 0;
    switch (reg)
    {
        case CTRL_STATUS:
            rv = 0;
            break;
        default:
            Log::e("Attempt to read from illegal APU register 0x%X (returned zero)", reg);
            assert(false);
    }
    return rv;
}

void APU::writeRegister(c6502_word_t reg, c6502_byte_t val)
{
    switch (reg)
    {
        case FRAME_SEQ:
            m_irqInhibit = val & 0x40u;
            m_5step = val & 0x80u;
            break;
        case CTRL_STATUS:
            m_pulse1.setEnabled(val & 0x01u);
            m_pulse2.setEnabled(val & 0x02u);
            m_tri.setEnabled(val & 0x04u);
            m_noise.setEnabled(val & 0x08u);
            m_dmc.setEnabled(val & 0x10u);
            break;
    }
}

void APU::runFrame()
{
    const int nClocks = bus().getMode() == OutputMode::NTSC ? divrnd(1789773, 60) :
                                                               divrnd(1662607, 50);

    assert(m_pBackend != nullptr);
    m_pBackend->setSampleCount(nClocks);

    // How much clocks to skip before triggering frame sequencer.
    // Need to align last clock with the last clock of the main timer, so
    // division is rounding to floor.
    const int fsPeriod = nClocks / (m_5step ? 5 : 4);
    int fsStep = 0;

    for (int c = 0; c < nClocks; c++)
    {
        // Clock frame sequencer. Skip immediate triggering at 0
        if (c > 0 && c % fsPeriod == 0)
        {
            const int step = c / fsPeriod;
            assert(step >= 1 && step <= 5);
            if (c != 5)
            {
                if ((m_5step && (step == 1 || step == 3)) ||
                    (!m_5step && (step == 2 || step == 4)))
                {
                    m_pulse1.clockLengthCounterSweep();
                    m_pulse2.clockLengthCounterSweep();
                }
                m_pulse1.envelope().clock();
                m_pulse1.envelope().clock();

                // TODO: trigger IRQ
                // if (!m_5step && c == 4)
            }
        }

        // Clock channel timers
        if (c % 2 != 0)
        {
            m_pulse1.clockTimer();
            m_pulse2.clockTimer();
        }
        m_tri.clockTimer();
        m_noise.clockTimer();
        m_dmc.clockTimer();

        // Read channel outputs
        const auto pul1 = m_pulse1.sample(),
                   pul2 = m_pulse2.sample(),
                   tri = m_tri.sample(),
                   nois = m_noise.sample(),
                   dmc = m_dmc.sample();

        // Combine outputs
        const float v = 95.88f / (8128.0f / (pul1 + pul2) + 100.0f) +
                        159.79f / (1.0f / ((tri / 8227.0f) + (nois / 12241.0f) + (dmc / 22638.0f)) + 100.0f);
        m_pBackend->queueSample(v);
    }
}
