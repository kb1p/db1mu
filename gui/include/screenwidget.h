#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QOpenGLWidget>

class ScreenWidget: public QOpenGLWidget
{
public:
    using QOpenGLWidget::QOpenGLWidget;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
};

#endif
