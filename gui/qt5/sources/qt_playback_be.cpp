#include "qt_playback_be.h"
#include <log.h>

#include <QAudioDeviceInfo>
#include <QAudioFormat>

QtPlaybackBackend::~QtPlaybackBackend()
{
    if (m_out)
        m_out->stop();
}

void QtPlaybackBackend::init() noexcept
{
    m_dev = nullptr;
    if (m_out)
        m_out->stop();
    else
    {
        m_frequency = 44100u;

        QAudioFormat format;
        format.setSampleRate(m_frequency);
        format.setChannelCount(1);
        format.setSampleSize(32);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::Float);

        // For now, pick any available device
        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        if (!info.isFormatSupported(format))
        {
            Log::e("Audio: required format not supported, audio playback disabled");
            return;
        }

        m_out = new QAudioOutput { format, this };
    }

    m_dev = m_out->start();
}

void QtPlaybackBackend::beginFrame(uint nSamples) noexcept
{
    m_sampleBuf.reserve(nSamples);
}

void QtPlaybackBackend::queueSample(float v) noexcept
{
    m_sampleBuf.push_back(v);
}

void QtPlaybackBackend::endFrame() noexcept
{
    if (m_dev)
        m_dev->write(reinterpret_cast<const char*>(m_sampleBuf.data()), m_sampleBuf.size() * 4);
    m_sampleBuf.clear();
}
