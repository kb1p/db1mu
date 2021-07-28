#include "cpustate.h"
#include "ui_cpu_state.h"

#include <cpu6502.h>

CPUStateDialog::CPUStateDialog(QWidget *parent):
    QDialog { parent }
{
    m_ui = new Ui::CPUStateDialog;
    m_ui->setupUi(this);
}

CPUStateDialog::~CPUStateDialog()
{
    delete m_ui;
}

void CPUStateDialog::show(const CPU6502 *pCPU)
{
    Q_ASSERT(pCPU);
    const auto regs = pCPU->registerStates();

    m_ui->txtPC->setText(QString::number(regs.pc, 16).rightJustified(4, '0'));
    m_ui->txtA->setText(QString::number(regs.a, 16).rightJustified(2, '0'));
    m_ui->txtX->setText(QString::number(regs.x, 16).rightJustified(2, '0'));
    m_ui->txtY->setText(QString::number(regs.y, 16).rightJustified(2, '0'));
    m_ui->txtS->setText(QString::number(regs.s, 16).rightJustified(2, '0'));

    QString flags = "";
    if (pCPU->getFlag<CPU6502::Flag::N>())
        flags.append('N');
    if (pCPU->getFlag<CPU6502::Flag::V>())
        flags.append('V');
    if (pCPU->getFlag<CPU6502::Flag::B>())
        flags.append('B');
    if (pCPU->getFlag<CPU6502::Flag::D>())
        flags.append('D');
    if (pCPU->getFlag<CPU6502::Flag::I>())
        flags.append('I');
    if (pCPU->getFlag<CPU6502::Flag::Z>())
        flags.append('Z');
    if (pCPU->getFlag<CPU6502::Flag::C>())
        flags.append('C');

    m_ui->txtFlags->setText(flags);

    m_ui->txtNMICount->setText(QString::number(pCPU->nmiCount()));
    m_ui->txtRTICount->setText(QString::number(pCPU->rtiCount()));
}

void CPUStateDialog::clear()
{
    m_ui->txtPC->clear();
    m_ui->txtA->clear();
    m_ui->txtX->clear();
    m_ui->txtY->clear();
    m_ui->txtS->clear();
    m_ui->txtFlags->clear();
    m_ui->txtNMICount->clear();
    m_ui->txtRTICount->clear();
}
