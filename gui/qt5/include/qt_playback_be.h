#ifndef QT_PLAYBACK_BE
#define QT_PLAYBACK_BE

#include <APU.h>
#include <QAudioOutput>
#include <QIODevice>
#include <QVector>

class QtPlaybackBackend: public QObject, public PlaybackBackend
{
    Q_OBJECT

    QAudioOutput *m_out = nullptr;
    QIODevice *m_dev = nullptr;
    QVector<float> m_sampleBuf;
    uint m_frequency = 0u;

public:
    using QObject::QObject;

    ~QtPlaybackBackend();

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
