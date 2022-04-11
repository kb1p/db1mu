#ifndef SDL_PLAYBACK_BE_H
#define SDL_PLAYBACK_BE_H

#include <APU.h>
#include <SDL2/SDL.h>
#include "ringbuffer.h"

class SDLPlaybackBackend: public PlaybackBackend
{
    SDL_AudioDeviceID m_devId = 0;
    RingBuffer<float> m_sampleBuf;
    float m_lastSample = 0;
    uint m_frequency = 0u;

    static void fillAudioBuffer(void *pUser, Uint8 *pBuf, int len) noexcept;

public:
    ~SDLPlaybackBackend();

    void init() noexcept override;

    uint getPlaybackFrequency() const noexcept override
    {
        assert(m_frequency != 0u);
        return m_frequency;
    }

    void beginFrame(uint nSamples) noexcept override;
    void queueSample(float v) noexcept override;
    void endFrame() noexcept override;
};

#endif
