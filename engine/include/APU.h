#ifndef APU_H
#define APU_H

#include "common.h"

// Interface that wraps platform-dependent playback subsystem
class PlaybackBackend
{
protected:
    PlaybackBackend() = default;

public:
    virtual ~PlaybackBackend() = default;

    virtual void init() noexcept = 0;
    virtual uint getPlaybackFrequency() const noexcept = 0;
    virtual void beginFrame(uint nSamples) noexcept = 0;
    virtual void queueSample(float v) noexcept = 0;
    virtual void endFrame() noexcept = 0;
};

// Envelope unit, common for Pulse and Noise channels
class Envelope
{
public:
    static constexpr uint ENVELOPE_VOLUME = 0x10u;

    void setVolume(uint cv) noexcept
    {
        m_vol = cv;
    }

    void setStartFlag(bool sf) noexcept
    {
        m_startFlag = sf;
    }

    void setDividerPeriod(uint p) noexcept
    {
        m_divPeriod = p;
    }

    void setLoop(bool loop) noexcept
    {
        m_loop = loop;
    }

    void clock() noexcept;

    uint volume() const noexcept
    {
        return m_vol < ENVELOPE_VOLUME ? m_vol : m_dlc;
    }

    void restart() noexcept
    {
        m_startFlag = true;
    }

private:
    bool m_startFlag = true,
         m_loop = false;
    uint m_divPeriod = 0u,
         m_divCnt = 0u,
         m_dlc = 0u,
         m_vol = 0x10u;
};

// Pulse channel
class PulseChannel
{
    static const uint SEQUENCER[4][8];

    bool m_enabled = false,
         m_lcHalt = false;
    uint m_lc = 0u,
         m_timerPeriod = 0u,
         m_duty = 0u,
         m_timerCnt = 0u,
         m_seqIndex = 0u;
    Envelope m_envelope;

    // Sweep
    bool m_swpReload = true,
         m_swpEnabled = false,
         m_swpNegate = false;
    const bool m_swpNegErr;
    uint m_swpPeriod = 0u,
         m_swpCounter = 0u,
         m_swpShift = 0u,
         m_swpTargetPeriod = 0u;

    void adjustPeriod() noexcept;

public:
    PulseChannel(bool swpNegErr):
        m_swpNegErr { swpNegErr }
    {
    }

    void setEnabled(bool e) noexcept
    {
        m_enabled = e;
        if (!m_enabled)
            m_lc = 0u;
    }

    void setLengthCounterHalt(bool lch) noexcept
    {
        m_lcHalt = lch;
    }

    void setLengthCounterLoad(uint l) noexcept;

    void setDuty(uint d) noexcept
    {
        m_duty = d;
    }

    void setSweepParams(bool enable, uint period, bool negate, uint shift) noexcept
    {
        m_swpEnabled = enable;
        m_swpPeriod = period;
        m_swpNegate = negate;
        m_swpShift = shift;
        m_swpReload = true;
    }

    void setTimerLo(c6502_byte_t v) noexcept
    {
        m_timerPeriod = v;
    }

    void setTimerHi(c6502_byte_t v) noexcept
    {
        m_timerPeriod |= v << 8u;
    }

    void clockTimer() noexcept;
    void clockLengthCounterSweep() noexcept;

    Envelope &envelope() noexcept
    {
        return m_envelope;
    }

    uint lengthCounter() const noexcept
    {
        return m_lc;
    }

    uint sample() noexcept;
};

class TriangleChannel
{
    bool m_enabled = false;
    uint m_lc = 0u;

public:
    void setEnabled(bool e) noexcept
    {
        m_enabled = e;
    }

    void clockTimer() noexcept { }
    uint sample() noexcept { return 0u; }
};

class NoiseChannel
{
    bool m_enabled = false;
    uint m_lc = 0u;

public:
    void setEnabled(bool e) noexcept
    {
        m_enabled = e;
    }

    void clockTimer() noexcept { }
    uint sample() noexcept { return 0u; }
};

class DMCChannel
{
    bool m_enabled = false;

public:
    void setEnabled(bool e) noexcept
    {
        m_enabled = e;
    }

    void clockTimer() noexcept { }
    uint sample() noexcept { return 0u; }
};

class APU: public Component
{
public:
    enum Register: c6502_word_t
    {
        RCT1_CTRL      = 0x0u,
        RCT1_GEN       = 0x1u,
        RCT1_FREQ_LO   = 0x2u,
        RCT1_FREQ_HI   = 0x3u,
        RCT2_CTRL      = 0x4u,
        RCT2_GEN       = 0x5u,
        RCT2_FREQ_LO   = 0x6u,
        RCT2_FREQ_HI   = 0x7u,
        TRI_CTRL       = 0x8u,
        TRI_FREQ1      = 0xAu,
        TRI_FREQ2      = 0xBu,
        NOIS_CTRL1     = 0xCu,
        NOIS_CTRL2     = 0xEu,
        NOIS_FREQ      = 0xFu,
        CTRL_STATUS    = 0x15u,
        FRAME_SEQ      = 0x17u
    };

    c6502_byte_t readRegister(c6502_word_t reg);
    void writeRegister(c6502_word_t reg, c6502_byte_t val);

    void runFrame();

    void setBackend(PlaybackBackend *ppbe) noexcept
    {
        m_pBackend = ppbe;
    }

    void reset() noexcept;

private:
    PlaybackBackend *m_pBackend = nullptr;

    bool m_5step = false,
         m_irqInhibit = false;

    PulseChannel m_pulse1 { true },
                 m_pulse2 { false };
    TriangleChannel m_tri;
    NoiseChannel m_noise;
    DMCChannel m_dmc;
};

#endif
