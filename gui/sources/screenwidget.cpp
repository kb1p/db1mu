#include "screenwidget.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

void ScreenWidget::initializeGL()
{
}

void ScreenWidget::resizeGL(int w, int h)
{
    const auto g = context()->functions();
    g->glClearColor(1, 0.5, 0, 1);
}

void ScreenWidget::paintGL()
{
    const auto g = context()->functions();
    g->glClear(GL_COLOR_BUFFER_BIT);
}
