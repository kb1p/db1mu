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
        reqFmt.samples = 512;
        reqFmt.callback = &SDLPlaybackBackend::fillAudioBuffer;
        reqFmt.userdata = this;

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
    SDL_LockAudioDevice(m_devId);
    m_sampleBuf.reserve(nSamples);
}

void SDLPlaybackBackend::queueSample(float v) noexcept
{
    m_lastSample = v;
    if (m_sampleBuf.isFull())
    {
        m_sampleBuf.reserve(std::max(m_sampleBuf.capacity() * 2u, 1u));
        Log::i("Audio: extended buffer size to %u", m_sampleBuf.capacity());
    }
    m_sampleBuf.enqueue(v);
}

void SDLPlaybackBackend::endFrame() noexcept
{
    SDL_UnlockAudioDevice(m_devId);
}

void SDLPlaybackBackend::fillAudioBuffer(void *pUser, Uint8 *pBuf, int len) noexcept
{
    auto thiz = static_cast<SDLPlaybackBackend*>(pUser);
    assert(len % 4 == 0);

    const uint nReq = len / 4,
               nHas = thiz->m_sampleBuf.size();
    if (nHas > 0)
        thiz->m_sampleBuf.dequeueRange(reinterpret_cast<float*>(pBuf), std::min(nReq, nHas));

    // Fill the remaining gap with repeat of the last sample
    for (uint off = nHas; off < nReq; off++)
        *(reinterpret_cast<float*>(pBuf) + off) = thiz->m_lastSample;
}
