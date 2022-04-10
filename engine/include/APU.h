#ifndef APU_H
#define APU_H

#include "common.h"
#include "bus.h"

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

// Common properties for all channels (length counter)
class APUChannel
{
    bool m_enabled = false,
         m_lenCntHalt = false;

    uint m_lenCnt = 0u,
         m_timerPeriod = 0u,
         m_timerCnt = 0u;

public:
    void setEnabled(bool e) noexcept
    {
        m_enabled = e;
        if (!m_enabled)
            m_lenCnt = 0u;
    }

    void setLengthCounterHalt(bool lch) noexcept
    {
        m_lenCntHalt = lch;
    }

    void setLengthCounterLoad(uint l) noexcept;

    void clockLengthCounter() noexcept;

    uint lengthCounter() const noexcept
    {
        return m_lenCnt;
    }

    void clockTimer() noexcept
    {
        if (m_timerCnt-- == 0u)
        {
            m_timerCnt = m_timerPeriod;
            onTimeout();
        }
    }

protected:
    virtual void onTimeout() noexcept = 0;

    void setTimerPeriod(uint tp) noexcept
    {
        m_timerPeriod = tp;
    }

    uint timerPeriod() const noexcept
    {
        return m_timerPeriod;
    }
};

// Pulse channel
class PulseChannel: public APUChannel
{
    static const uint SEQUENCER[4][8];

    uint m_duty = 0u,
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

public:
    PulseChannel(bool swpNegErr):
        m_swpNegErr { swpNegErr }
    {
    }

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

    void setTimerLo(uint v) noexcept
    {
        setTimerPeriod(v);
    }

    void setTimerHi(uint v) noexcept
    {
        setTimerPeriod(timerPeriod() | (v << 8u));
    }

    void restartSequencer() noexcept
    {
        m_seqIndex = 0u;
    }

    void clockSweep() noexcept;

    Envelope &envelope() noexcept
    {
        return m_envelope;
    }

    uint sample() noexcept;

protected:
    void onTimeout() noexcept override;
};

class TriangleChannel: public APUChannel
{
    static const uint SEQUENCER[32];

    bool m_linCntReload = false,
         m_linCntControl = false;
    uint m_seqIndex = 0u,
         m_linCntSet = 0u,
         m_linCnt = 0u;

public:
    void setTimerLo(uint v) noexcept
    {
        setTimerPeriod(v);
    }

    void setTimerHi(uint v) noexcept
    {
        setTimerPeriod(timerPeriod() | (v << 8u));
    }

    void setLinearCounter(bool ctrl, uint linCtr) noexcept
    {
        m_linCntControl = ctrl;
        m_linCntSet = linCtr;
        setLengthCounterHalt(ctrl);
    }

    void setLinearCounterReloadFlag() noexcept
    {
        m_linCntReload = true;
    }

    void clockLinearCounter() noexcept;
    uint sample() noexcept;

protected:
    void onTimeout() noexcept override;
};

class NoiseChannel: public APUChannel
{
    static const uint PERIOD_MAP_PAL[16],
                      PERIOD_MAP_NTSC[16];

    OutputMode m_mode = OutputMode::NTSC;
    Envelope m_envelope;
    uint m_shift = 1u;
    bool m_loop = false;

public:
    void setOutputMode(OutputMode m) noexcept
    {
        m_mode = m;
        m_shift = 1u;
    }

    void setPeriod(uint period) noexcept
    {
        assert(period <= 0x0Fu);
        setTimerPeriod((m_mode == OutputMode::NTSC ? PERIOD_MAP_NTSC : PERIOD_MAP_PAL)[period]);
    }

    void setLoop(bool loop) noexcept
    {
        m_loop = loop;
    }

    Envelope &envelope() noexcept
    {
        return m_envelope;
    }

    uint sample() noexcept;

protected:
    void onTimeout() noexcept override;
};

// DMC - Delta Modulation Channel
// Plays sampled audio data (not available on Dendy).
class DMChannel: public APUChannel
{
public:
    uint sample() noexcept { return 0u; }

protected:
    void onTimeout() noexcept override
    {
    }
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
    DMChannel m_dmc;
};

#endif
