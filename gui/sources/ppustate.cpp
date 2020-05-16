#include "ppustate.h"

#include "ui_ppu_state.h"
#include <PPU.h>
#include <QLabel>

PPUStateDialog::PPUStateDialog(QWidget *parent):
    QDialog { parent }
{
    m_ui = new Ui::PPUStateDialog;
    m_ui->setupUi(this);

    m_pmOn.fill(Qt::green);
    m_pmOff.fill(Qt::red);
}

PPUStateDialog::~PPUStateDialog()
{
    delete m_ui;
}

void PPUStateDialog::setIndicator(QLabel *l, bool val)
{
    l->setPixmap(val ? m_pmOn : m_pmOff);
}

void PPUStateDialog::show(const PPU *pPPU)
{
    const auto st = pPPU->currentState();

    setIndicator(m_ui->txtNMIEnabled, st.enableNMI);
    setIndicator(m_ui->txtWriteEnabled, st.enableWrite);
    setIndicator(m_ui->txtBgVisible, st.backgroundVisible);
    setIndicator(m_ui->txtSpritesVisible, st.spritesVisible);
    setIndicator(m_ui->txtBigSprites, st.bigSprites);
    setIndicator(m_ui->txtAllSprites, st.allSpritesVisible);
    setIndicator(m_ui->txtFullBg, st.fullBacgroundVisible);
    setIndicator(m_ui->txtVblank, st.vblank);
    setIndicator(m_ui->txtOver8, st.over8sprites);
    setIndicator(m_ui->txtSpr0, st.sprite0);

    m_ui->txtBgAddr->setText(QString::number(st.baBkgnd, 16).toUpper().rightJustified(4, '0'));
    m_ui->txtSprAddr->setText(QString::number(st.baSprites, 16).toUpper().rightJustified(4, '0'));
    m_ui->txtActivePage->setText(QString::number(st.activePage(), 16).toUpper().rightJustified(4, '0'));
    m_ui->txtVmemAddr->setText(QString::number(st.vramAddr, 16).toUpper().rightJustified(4, '0'));
    m_ui->txtAddrIncr->setText(QString::number(st.addrIncr, 16).toUpper().rightJustified(4, '0'));
    m_ui->txtSprMemAddr->setText(QString::number(st.sprmemAddr, 16).toUpper().rightJustified(4, '0'));
    m_ui->txtVscroll->setText(QString::number(st.scrollV));
    m_ui->txtHscroll->setText(QString::number(st.scrollH));
}

void PPUStateDialog::clear()
{
    m_ui->txtNMIEnabled->clear();
    m_ui->txtWriteEnabled->clear();
    m_ui->txtBgVisible->clear();
    m_ui->txtSpritesVisible->clear();
    m_ui->txtBigSprites->clear();
    m_ui->txtAllSprites->clear();
    m_ui->txtFullBg->clear();
    m_ui->txtVblank->clear();
    m_ui->txtOver8->clear();
    m_ui->txtBgAddr->clear();
    m_ui->txtSprAddr->clear();
    m_ui->txtActivePage->clear();
    m_ui->txtVmemAddr->clear();
    m_ui->txtAddrIncr->clear();
    m_ui->txtSprMemAddr->clear();
    m_ui->txtVscroll->clear();
    m_ui->txtHscroll->clear();
    m_ui->txtSpr0->clear();
}
