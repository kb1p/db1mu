#ifndef PPU_STATE_H
#define PPU_STATE_H

#include <QDialog>
#include <QPixmap>

class PPU;

namespace Ui
{
    class PPUStateDialog;
}

class PPUStateDialog: public QDialog
{
public:
    PPUStateDialog(QWidget *parent);
    ~PPUStateDialog();

    void show(const PPU *pPPU);
    void clear();

private:
    Ui::PPUStateDialog *m_ui = nullptr;
    QPixmap m_pmOn { 16, 16 },
            m_pmOff { 16, 16 };

    void setIndicator(class QLabel *l, bool val);
};

#endif
