/*
 * Copyright (c) 2015, <copyright holder> <email>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "b1mainwindow.h"
#include "ui_b1mainwindow.h"
#include "b1.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileDialog>

b1MainWindow::b1MainWindow()
{
    ui = new Ui::b1MainWindow;
    ui->setupUi ( this );

    m_screen = new ScreenWidget(this);
    setCentralWidget(m_screen);
}

b1MainWindow::~b1MainWindow()
{
    delete ui;
}

void b1MainWindow::closeEvent(QCloseEvent *e)
{
    const auto r = QMessageBox::question(this,
                                         tr("Confirm exit"),
                                         tr("Are you sure want to quit?"),
                                         QMessageBox::Yes | QMessageBox::No);
    if (r == QMessageBox::Yes)
    {
        e->accept();
        //foo();
    }
    else
    {
        e->ignore();
    }
}

void b1MainWindow::openROM()
{
    const auto fn = QFileDialog::getOpenFileName(this,
                                                 tr("Select ROM file"),
                                                 tr("."),
                                                 tr("NES ROM images (*.nes)"));
    if (!fn.isNull())
        m_screen->loadROM(fn);
}

