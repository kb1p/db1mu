#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QWindow>
#include <QWidget>
#include <QElapsedTimer>
#ifdef USE_VULKAN
    #include <QVulkanInstance>
#else
    #include <QOpenGLContext>
#endif
#include <memory>

class Bus;
class RenderingBackend;

class ScreenWidget: public QWindow
{
    Q_OBJECT

public:
    ScreenWidget(QWidget *container);
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
    bool event(QEvent *e) override;

private:
    QWidget *const m_pContainer;

    Bus *m_pBus = nullptr;
    int m_timerId = 0;
    std::unique_ptr<RenderingBackend> m_RBE;
    bool m_runEmulation = false;

    QElapsedTimer m_clocks;
    int m_accFrameTimes = 0,
        m_nFrames = 0;

#ifdef USE_VULKAN
    QVulkanInstance m_vkInstance;
#else
    QOpenGLContext *m_pGLCtx = nullptr;
#endif

    void initialize();
    void render();
};

#endif
