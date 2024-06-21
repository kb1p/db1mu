#include "screenwidget.h"

#include <bus.h>
#include <QSurfaceFormat>
#include <QMessageBox>
#include <QTimerEvent>
#include <QDebug>

#ifdef USE_VULKAN
    #include "vkrbe.h"

    using Backend = VulkanRenderingBackend;
#else
    #include <QOpenGLFunctions>
    #include "glbe.h"

    using Backend = GLRenderingBackend<QOpenGLFunctions>;
#endif

ScreenWidget::ScreenWidget(QWidget *container):
    m_pContainer { container }
{
#ifdef USE_VULKAN
    setSurfaceType(QWindow::VulkanSurface);
#else
    setSurfaceType(QWindow::OpenGLSurface);

    // Request alpha channel and depth buffer
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    fmt.setVersion(2, 0);
    fmt.setRedBufferSize(2);
    fmt.setGreenBufferSize(2);
    fmt.setBlueBufferSize(2);
    fmt.setAlphaBufferSize(1);

    setFormat(fmt);
    //setUpdateBehavior(QOpenGLWidget::PartialUpdate);

    m_pGLCtx = new QOpenGLContext { this };
    m_pGLCtx->setFormat(fmt);
#endif

    m_RBE.reset(new Backend);

    m_timerId = startTimer(17, Qt::PreciseTimer);
}

ScreenWidget::~ScreenWidget()
{
    m_RBE.reset();
}

bool ScreenWidget::isRunning() const noexcept
{
    return m_runEmulation;
}

void ScreenWidget::pause()
{
    Q_ASSERT(m_timerId != 0);
    m_runEmulation = false;
    killTimer(m_timerId);
    m_timerId = 0;
}

void ScreenWidget::resume()
{
    m_nFrames = m_accFrameTimes = 0;
    m_runEmulation = true;
    m_clocks.start();
    if (m_timerId == 0)
        m_timerId = startTimer(17, Qt::PreciseTimer);
}

void ScreenWidget::step()
{
    m_nFrames = m_accFrameTimes = 0;
    m_runEmulation = true;
    requestUpdate();
    m_runEmulation = false;
}

void ScreenWidget::initialize()
{
#ifdef USE_VULKAN
    if (m_vkInstance.isValid())
        return;

    try
    {
        if (!m_vkInstance.create())
            throw Exception { Exception::IllegalOperation, "failed to create Vulkan instance" };

        setVulkanInstance(&m_vkInstance);
        auto pBE = static_cast<Backend*>(m_RBE.get());
        pBE->init(m_vkInstance.vkInstance());
        pBE->setupOutput(QVulkanInstance::surfaceForWindow(this));
    }
    catch (Exception &ex)
    {
        QMessageBox::critical(m_pContainer,
                              tr("GLES init failed"),
                              tr("Error while initializing GLES: %1").arg(ex.message()));
    }
#else
    if (m_pGLCtx->isValid())
        return;

    try
    {
        if (!m_pGLCtx->create())
            throw Exception { Exception::IllegalOperation, "failed to create OpenGL context" };

        m_pGLCtx->makeCurrent(this);

        const static auto reqFeatures = {
            QOpenGLFunctions::Shaders
        };

        const auto glFuncs = m_pGLCtx->functions();
        const auto feats = glFuncs->openGLFeatures();
        for (auto f: reqFeatures)
            if ((feats & f) == 0)
                throw Exception { Exception::IllegalOperation, "required OpenGL features are not supported" };

        static_cast<Backend*>(m_RBE.get())->init(glFuncs);
    }
    catch (Exception &ex)
    {
        QMessageBox::critical(m_pContainer,
                              tr("GLES init failed"),
                              tr("Error while initializing GLES: %1").arg(ex.message()));
    }
#endif
}

void ScreenWidget::render()
{
#ifdef USE_VULKAN
    if (!m_vkInstance.isValid())
        return;
#else
    if (!m_pGLCtx->isValid())
        return;

    m_pGLCtx->makeCurrent(this);
#endif

    Q_ASSERT(m_pBus);
    if (m_runEmulation)
    {
        const int dt = m_clocks.restart();
        if (m_nFrames++ > 0)
        {
            if (m_nFrames < 60)
                m_accFrameTimes += dt;
            else
            {
                Q_EMIT fpsChanged(m_nFrames * 1000.0f / m_accFrameTimes);
                m_nFrames = 1;
                m_accFrameTimes = dt;
            }
        }

        m_pBus->runFrame();
    }
    else if (!m_pBus->getCartrige())
    {
        m_RBE->drawIdle();
    }

#ifndef USE_VULKAN
    m_pGLCtx->swapBuffers(this);
#endif
}

bool ScreenWidget::event(QEvent *e)
{
    bool rv = true;
    try
    {
        switch (e->type())
        {
            case QEvent::Expose:
                if (!isExposed())
                {
                    rv = QWindow::event(e);
                    break;
                }
                initialize();
            case QEvent::UpdateRequest:
                render();
                //requestUpdate();
                break;
            case QEvent::Resize:
                static_cast<Backend*>(m_RBE.get())->resize(width(), height());
                break;
            case QEvent::Timer:
                if (static_cast<QTimerEvent*>(e)->timerId() == m_timerId)
                    requestUpdate();
                break;
            default:
                rv = QWindow::event(e);
        }
    }
    catch (const std::exception &ex)
    {
        QMessageBox::critical(m_pContainer,
                              tr("Emulator window error"),
                              tr("Error: %1").arg(ex.what()));
        rv = false;
    }

    return rv;
}
