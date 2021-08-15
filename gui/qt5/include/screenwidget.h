#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QOpenGLWidget>
#include <QElapsedTimer>
#include <QOpenGLFunctions>
#include <memory>

class Bus;
class RenderingBackend;

class ScreenWidget: public QOpenGLWidget
{
    Q_OBJECT

public:
    ScreenWidget(QWidget *parent = nullptr);
    ~ScreenWidget();

    void setBus(Bus *pBus) noexcept
    {
        Q_ASSERT(pBus != nullptr);
        m_pBus = pBus;
    }

    void pause();
    void step();
    void resume();

    bool isRunning() const noexcept;

    RenderingBackend *getRenderingBackend() const noexcept
    {
        return m_RBE.get();
    }

Q_SIGNALS:
    void fpsChanged(float);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void timerEvent(QTimerEvent *event) override;

private:
    Bus *m_pBus = nullptr;
    int m_timerId = 0;
    std::unique_ptr<RenderingBackend> m_RBE;
    bool m_runEmulation = false;

    QElapsedTimer m_clocks;
    int m_accFrameTimes = 0,
        m_nFrames = 0;
};

#endif
