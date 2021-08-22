#ifndef SDL_PLAYBACK_BE_H
#define SDL_PLAYBACK_BE_H

#include <APU.h>
#include <SDL2/SDL.h>
#include <vector>

class SDLPlaybackBackend: public PlaybackBackend
{
    SDL_AudioDeviceID m_devId = 0;
    std::vector<float> m_sampleBuf;
    uint m_frequency = 0u;

public:
    ~SDLPlaybackBackend();

    void init() noexcept override;

    uint getPlaybackFrequency() const noexcept override
    {
        assert(m_frequency != 0u);
        return m_frequency;
    }

    void beginFrame(uint nSamples) noexcept override;
    void queueSample(float v) override;
    void endFrame() noexcept override;
};

#endif
