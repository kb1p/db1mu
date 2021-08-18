#ifndef APU_H
#define APU_H

#include "common.h"

class PlaybackBackend
{
protected:
    PlaybackBackend() = default;

public:
    virtual ~PlaybackBackend() = default;

    virtual void setSampleCount(int sc) = 0;
    virtual void queueSample(float v) = 0;
};

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

private:
    bool m_startFlag = true,
         m_loop = false;
    uint m_divPeriod = 0u,
         m_divCnt = 0u,
         m_dlc = 0u,
         m_vol = 0x10u;
};

class PulseChannel
{
    static const uint SEQUENCER[4][8];

    // Setup values
    bool m_enabled = false;
    uint m_lc = 0u,
         m_timerSetup = 0u,
         m_dutySetup = 0u;

    // Current state
    uint m_timer = 0u,
         m_seqIndex = 0u;
    Envelope m_envelope;

public:
    void setEnabled(bool e) noexcept
    {
        m_enabled = e;
    }

    void clockTimer() noexcept;
    void clockLengthCounterSweep() noexcept;

    Envelope &envelope() noexcept
    {
        return m_envelope;
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
        RCT1_CTRL    = 0x0u,
        RCT1_GEN     = 0x1u,
        RCT1_FREQ1   = 0x2u,
        RCT1_FREQ2   = 0x3u,
        RCT2_CTRL    = 0x4u,
        RCT2_GEN     = 0x5u,
        RCT2_FREQ1   = 0x6u,
        RCT2_FREQ2   = 0x7u,
        TRI_CTRL     = 0x8u,
        TRI_FREQ1    = 0xAu,
        TRI_FREQ2    = 0xBu,
        NOIS_CTRL1   = 0xCu,
        NOIS_CTRL2   = 0xEu,
        NOIS_FREQ    = 0xFu,
        CTRL_STATUS  = 0x15u,
        FRAME_SEQ    = 0x17u
    };

    c6502_byte_t readRegister(c6502_word_t reg);
    void writeRegister(c6502_word_t reg, c6502_byte_t val);

    void runFrame();

    void setBackend(PlaybackBackend *ppbe) noexcept
    {
        m_pBackend = ppbe;
    }

private:
    PlaybackBackend *m_pBackend = nullptr;

    bool m_5step = false,
         m_irqInhibit = false;

    PulseChannel m_pulse1, m_pulse2;
    TriangleChannel m_tri;
    NoiseChannel m_noise;
    DMCChannel m_dmc;
};

#endif
