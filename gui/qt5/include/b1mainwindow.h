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

#ifndef B1MAINWINDOW_H
#define B1MAINWINDOW_H

#include <QMainWindow>
#include "screenwidget.h"
#include "cpustate.h"
#include "ppustate.h"
#include <memory>

namespace Ui
{
class b1MainWindow;
}

struct NESEngine;

class b1MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    b1MainWindow();
    ~b1MainWindow();

protected:
    void closeEvent(QCloseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

    void loadROM(const QString &romName);
    void updateUI();

protected Q_SLOTS:
    void openROM();
    void pauseEmulation();
    void resumeEmulation();
    void stepEmulation();
    void fpsUpdated(float fps);
    void saveState();
    void loadState();

private:
    Ui::b1MainWindow *m_ui = nullptr;
    ScreenWidget *m_screen = nullptr;
    CPUStateDialog *m_cpuState = nullptr;
    PPUStateDialog *m_ppuState = nullptr;

    std::unique_ptr<NESEngine> m_eng;
};

#endif // B1MAINWINDOW_H
