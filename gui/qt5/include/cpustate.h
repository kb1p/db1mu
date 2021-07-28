#ifndef CPU_STATE_H
#define CPU_STATE_H

#include <QDialog>

class CPU6502;

namespace Ui
{
    class CPUStateDialog;
}

class CPUStateDialog: public QDialog
{
public:
    CPUStateDialog(QWidget *parent);
    ~CPUStateDialog();

    void show(const CPU6502 *pCPU);
    void clear();

private:
    Ui::CPUStateDialog *m_ui = nullptr;
};

#endif
