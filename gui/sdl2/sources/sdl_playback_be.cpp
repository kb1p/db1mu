#include "sdl_playback_be.h"
#include <log.h>
#include <cstdlib>

SDLPlaybackBackend::~SDLPlaybackBackend()
{
    if (m_devId > 0)
        SDL_CloseAudioDevice(m_devId);
}

void SDLPlaybackBackend::init() noexcept
{
    if (m_devId == 0)
    {
        m_frequency = 44100u;

        // For now, pick any available device
        SDL_AudioSpec reqFmt, outFmt;
        SDL_zero(reqFmt);
        reqFmt.freq = m_frequency;
        reqFmt.format = AUDIO_F32;
        reqFmt.channels = 1;
        reqFmt.samples = 4096;

        const auto n = SDL_GetNumAudioDevices(SDL_FALSE);

        //Open playback device
        m_devId = SDL_OpenAudioDevice(nullptr,
                                      SDL_FALSE,
                                      &reqFmt,
                                      &outFmt,
                                      SDL_AUDIO_ALLOW_FORMAT_CHANGE);

        assert(outFmt.format == AUDIO_F32);
        assert(outFmt.channels == 1);
        assert(m_devId != 0);
        m_frequency = outFmt.freq;

        SDL_PauseAudioDevice(m_devId, SDL_FALSE);
    }
}

void SDLPlaybackBackend::beginFrame(uint nSamples) noexcept
{
    m_sampleBuf.reserve(nSamples);
}

void SDLPlaybackBackend::queueSample(float v)
{
    m_sampleBuf.push_back(v);
}

void SDLPlaybackBackend::endFrame() noexcept
{
    SDL_QueueAudio(m_devId, m_sampleBuf.data(), m_sampleBuf.size() * 4);
    m_sampleBuf.clear();
}
