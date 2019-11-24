#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QOpenGLWidget>

class ScreenWidget: public QOpenGLWidget
{
public:
    ScreenWidget(QWidget *parent = nullptr);
    ~ScreenWidget();

    void loadROM(const QString &fn);
    void reset(bool soft);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void timerEvent(QTimerEvent *event) override;

private:
    struct NESEngine *m_pEng = nullptr;
    int m_timerId = 0;
};

#endif
