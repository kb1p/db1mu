#include "screenwidget.h"
#include "glbe.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QMessageBox>
#include <QTimerEvent>
#include <QDebug>

#include <bus.h>

ScreenWidget::ScreenWidget(QWidget *parent):
    QOpenGLWidget { parent }
{
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

    m_pRBE = new GLRenderingBackend;
}

ScreenWidget::~ScreenWidget()
{
    delete m_pRBE;
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
    Q_ASSERT(m_timerId == 0);
    m_nFrames = m_accFrameTimes = 0;
    m_runEmulation = true;
    m_clocks.start();
    m_timerId = startTimer(17, Qt::PreciseTimer);
}

void ScreenWidget::step()
{
    m_nFrames = m_accFrameTimes = 0;
    m_runEmulation = true;
    repaint();
    m_runEmulation = false;
}

void ScreenWidget::initializeGL()
{
    try
    {
        m_pRBE->init(context()->functions());
    }
    catch (Exception &ex)
    {
        QMessageBox::critical(this,
                              tr("GLES init failed"),
                              tr("Error while initializing GLES: %1").arg(ex.message()));
    }
}

void ScreenWidget::resizeGL(int w, int h)
{
}

void ScreenWidget::timerEvent(QTimerEvent *event)
{
    Q_ASSERT(event->timerId() == m_timerId);
    repaint();
}

void ScreenWidget::paintGL()
{
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
        const auto g = context()->functions();
        g->glClearColor(1, 1, 1, 1);
        g->glClear(GL_COLOR_BUFFER_BIT);
    }
}
