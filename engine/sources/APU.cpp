#include "APU.h"
#include "log.h"
#include "bus.h"

#include <cassert>

// Length counter decipher table
static constexpr uint LC_TABLE[32] = {
    10,  // 0
    254, // 1
    20,  // 2
    2,   // 3
    40,  // 4
    4,   // 5
    80,  // 6
    6,   // 7
    160, // 8
    8,   // 9
    60,  // A
    10,  // B
    14,  // C
    12,  // D
    26,  // E
    14,  // F
    12,  // 10
    16,  // 11
    24,  // 12
    18,  // 13
    48,  // 14
    20,  // 15
    96,  // 16
    22,  // 17
    192, // 18
    24,  // 19
    72,  // 1A
    26,  // 1B
    16,  // 1C
    28,  // 1D
    32,  // 1E
    30   // 1F
};

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
        if (m_dlc > 0u)
            m_dlc--;
        else if (m_loop)
            m_dlc = 15u;
    }
}

void APUChannel::setLengthCounterLoad(uint l) noexcept
{
    assert(l < 32);
    if (m_enabled)
        m_lenCnt = LC_TABLE[l];
}

void APUChannel::clockLengthCounter() noexcept
{
    if (!m_lenCntHalt && m_lenCnt > 0u)
        m_lenCnt--;
}

const uint PulseChannel::SEQUENCER[4][8] = {
    { 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 0, 0, 1, 1, 1, 1, 1 }
};

void PulseChannel::clockTimer() noexcept
{
    if (m_timerCnt-- == 0u)
    {
        m_timerCnt = m_timerPeriod;

        // Trigger sequencer switch
        m_seqIndex = (m_seqIndex + 1) % 8;
    }
}

void PulseChannel::clockSweep() noexcept
{
    m_swpCounter--;
    const uint x = m_timerPeriod >> m_swpShift;
    m_swpTargetPeriod = m_timerPeriod + (m_swpNegate ? -(m_swpNegErr ? x - 1 : x) : x);

    if (m_swpReload)
    {
        m_swpReload = false;
        m_swpCounter = m_swpPeriod;
    }
    else if (m_swpCounter == 0u)
    {
        m_swpCounter = m_swpPeriod;
        if (m_swpEnabled &&
            m_swpShift > 0u &&
            m_timerPeriod >= 8u &&
            m_swpTargetPeriod <= 0x7ffu)
            m_timerPeriod = m_swpTargetPeriod;
    }
}

uint PulseChannel::sample() noexcept
{
    uint rv = 0u;

    if (m_timerPeriod >= 8u &&
        m_swpTargetPeriod <= 0x7ffu &&
        lengthCounter() > 0u &&
        SEQUENCER[m_duty][m_seqIndex] > 0u)
        rv = m_envelope.volume();

    return rv;
}

const uint TriangleChannel::SEQUENCER[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

void TriangleChannel::clockLinearCounter() noexcept
{
    if (m_linCntReload)
        m_linCnt = m_linCntSet;
    else if (m_linCnt > 0u)
        m_linCnt--;

    if (!m_linCntControl)
        m_linCntReload = false;
}

void TriangleChannel::clockTimer() noexcept
{
    if (m_timerCnt-- == 0u)
    {
        m_timerCnt = m_timerPeriod;

        // Trigger sequencer switch if both counters are non-zero
        if (lengthCounter() > 0u && m_linCnt > 0u)
            m_seqIndex = (m_seqIndex + 1) % 32;
    }
}

uint TriangleChannel::sample() noexcept
{
    return SEQUENCER[m_seqIndex];
}

c6502_byte_t APU::readRegister(c6502_word_t reg)
{
    c6502_byte_t rv = 0;
    switch (reg)
    {
        case CTRL_STATUS:
            rv |= m_pulse1.lengthCounter() > 0u ? 0b001u : 0u;
            rv |= m_pulse2.lengthCounter() > 0u ? 0b010u : 0u;
            rv |= m_tri.lengthCounter() > 0u ? 0b100u : 0u;
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
        case RCT1_CTRL:
            {
                auto &ev = m_pulse1.envelope();
                if (val & 0b10000u)
                    ev.setVolume(val & 0b1111u);
                else
                {
                    ev.setVolume(Envelope::ENVELOPE_VOLUME);
                    ev.setDividerPeriod((val & 0b1111u) + 1u);
                }
                ev.setLoop(val & 0b100000u);
                m_pulse1.setLengthCounterHalt(val & 0b100000u);
                m_pulse1.setDuty((val & 0b11000000u) >> 6u);
            }
            break;
        case RCT1_GEN:
            m_pulse1.setSweepParams(val & 0x80u,
                                    ((val & 0x70u) >> 4u) + 1,
                                    val & 0x8u,
                                    val & 0x7u);
            break;
        case RCT1_FREQ_LO:
            m_pulse1.setTimerLo(val);
            break;
        case RCT1_FREQ_HI:
            m_pulse1.setTimerHi(val & 0b111u);
            m_pulse1.setLengthCounterLoad((val >> 3u) & 0b11111u);
            m_pulse1.envelope().restart();
            m_pulse1.restartSequencer();
            break;
        case RCT2_CTRL:
            {
                auto &ev = m_pulse2.envelope();
                if (val & 0b10000u)
                    ev.setVolume(val & 0b1111u);
                else
                {
                    ev.setVolume(Envelope::ENVELOPE_VOLUME);
                    ev.setDividerPeriod((val & 0b1111u) + 1u);
                }
                ev.setLoop(val & 0b100000u);
                m_pulse2.setLengthCounterHalt(val & 0b100000u);
                m_pulse2.setDuty((val & 0b11000000u) >> 6u);
            }
            break;
        case RCT2_GEN:
            m_pulse2.setSweepParams(val & 0x80u,
                                    ((val & 0x70u) >> 4u) + 1u,
                                    val & 0x8u,
                                    val & 0x7u);
            break;
        case RCT2_FREQ_LO:
            m_pulse2.setTimerLo(val);
            break;
        case RCT2_FREQ_HI:
            m_pulse2.setTimerHi(val & 0b111u);
            m_pulse2.setLengthCounterLoad((val >> 3u) & 0b11111u);
            m_pulse2.envelope().restart();
            m_pulse2.restartSequencer();
            break;
        case TRI_CTRL:
            m_tri.setLinearCounter(val & 0x80u, val & 0x7Fu);
            break;
        case TRI_FREQ1:
            m_tri.setTimerLo(val);
            break;
        case TRI_FREQ2:
            m_tri.setTimerHi(val & 0b111u);
            m_tri.setLengthCounterLoad((val >> 3u) & 0b11111u);
            m_tri.setLinearCounterReloadFlag();
            break;
    }
}

void APU::runFrame()
{
    assert(m_pBackend != nullptr);

    const auto nClocks = bus().getMode() == OutputMode::NTSC ?
                         divrnd(1789773u, 60u) :
                         divrnd(1662607u, 50u);
    const auto nRecvClocks = bus().getMode() == OutputMode::NTSC ?
                             divrnd(m_pBackend->getPlaybackFrequency(), 60u) :
                             divrnd(m_pBackend->getPlaybackFrequency(), 50u);
    const auto sampleRate = divrnd(nClocks, nRecvClocks);

    // How much clocks to skip before triggering frame sequencer.
    // Need to align last clock with the last clock of the main timer, so
    // division is rounding to floor.
    const int fsPeriod = divrnd(nClocks, m_5step ? 5 : 4);
    int fsStep = 0;
    m_pBackend->beginFrame(nClocks / sampleRate);
    for (uint c = 0; c < nClocks; c++)
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
                    m_pulse1.clockLengthCounter();
                    m_pulse1.clockSweep();
                    m_pulse2.clockLengthCounter();
                    m_pulse2.clockSweep();
                    m_tri.clockLengthCounter();
                }
                m_pulse1.envelope().clock();
                m_pulse2.envelope().clock();
                m_tri.clockLinearCounter();

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

        if (c % sampleRate == 0u)
        {
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
    m_pBackend->endFrame();
}

void APU::reset() noexcept
{
    assert(m_pBackend != nullptr);
    m_pBackend->init();

    m_pulse1.setEnabled(false);
    m_pulse2.setEnabled(false);
    m_tri.setEnabled(false);
    m_noise.setEnabled(false);
    m_dmc.setEnabled(false);
}
