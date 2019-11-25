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
    fmt.setDepthBufferSize(16);

    setFormat(fmt);

    m_pRBE = new GLRenderingBackend;
}

ScreenWidget::~ScreenWidget()
{
    delete m_pRBE;
}

bool ScreenWidget::isRunning() const noexcept
{
    return m_pBus->getCartrige() != nullptr && m_timerId != 0;
}

void ScreenWidget::pause()
{
    Q_ASSERT(m_timerId != 0);
    killTimer(m_timerId);
    m_timerId = 0;
}

void ScreenWidget::resume()
{
    Q_ASSERT(m_timerId == 0);
    m_timerId = startTimer(17, Qt::PreciseTimer);
}

void ScreenWidget::step()
{
    repaint();
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
    if (m_pBus->getCartrige() != nullptr)
        m_pBus->runFrame();
    else
    {
        const auto g = context()->functions();
        g->glClearColor(1, 1, 1, 1);
        g->glClear(GL_COLOR_BUFFER_BIT);
    }
}
