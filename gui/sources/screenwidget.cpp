#include "screenwidget.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QMessageBox>
#include <QTimerEvent>

#include <bus.h>
#include <cpu6502.h>
#include <PPU.h>
#include <Cartridge.h>
#include <loader.h>

#include "glbe.h"

struct NESEngine
{
     Bus bus;
     CPU6502 cpu;
     GLRenderingBackend renderer;
     PPU ppu;
     Cartrige cartridge;
     bool ready = false;

     NESEngine(OutputMode mode):
        bus { mode },
        cpu { bus },
        ppu { bus, &renderer }
    {
        bus.setCPU(&cpu);
        bus.setPPU(&ppu);
    }
};

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

    m_pEng = new NESEngine { OutputMode::NTSC };
}

ScreenWidget::~ScreenWidget()
{
    delete m_pEng;
}

void ScreenWidget::loadROM(const QString &fn)
{
    if (m_timerId != 0)
        killTimer(m_timerId);

    m_pEng->ready = false;
    ROMLoader loader(m_pEng->cartridge);
    try
    {
        loader.loadNES(fn.toLocal8Bit().data());
        m_pEng->bus.injectCartrige(&m_pEng->cartridge);
        m_pEng->ready = true;
        m_timerId = startTimer(17, Qt::PreciseTimer);
    }
    catch (const Exception &ex)
    {
        QMessageBox::critical(this,
                              tr("Cannot load ROM"),
                              tr("Error: %1").arg(ex.message()));
    }
}

void ScreenWidget::initializeGL()
{
    try
    {
        m_pEng->renderer.init(context()->functions());
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
    if (event->timerId() == m_timerId)
        repaint();
}

void ScreenWidget::paintGL()
{
    if (m_pEng->ready)
    {
        m_pEng->cpu.runFrame();
        m_pEng->ppu.update();
    }
    else
    {
        const auto g = context()->functions();
        g->glClearColor(1, 1, 1, 1);
        g->glClear(GL_COLOR_BUFFER_BIT);
    }
}
