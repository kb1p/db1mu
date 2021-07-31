#include "mainwindow.h"

#include <log.h>
#include <loader.h>
#include <algorithm>
#include <iostream>

void MainWindow::initialize(const char *romFileName)
{
    auto &logCfg = Log::instance().config();
    logCfg.pOutput = &std::cout;
    logCfg.filter = Log::LEVEL_DEBUG;
    logCfg.autoFlush = true;

    m_renderingBackend.reset(new GLRenderingBackend<GLFunctionsWrapper>);
    m_renderingBackend->init(&m_glFuncWrp);

    m_bus.reset(new Bus { OutputMode::NTSC });
    m_cpu.reset(new CPU6502);
    m_ppu.reset(new PPU { m_renderingBackend.get() });
    m_cartridge.reset(new Cartrige);
    m_padLeft.reset(new Gamepad);
    m_padRight.reset(new Gamepad);

    m_bus->setCPU(m_cpu.get());
    m_bus->setPPU(m_ppu.get());
    m_bus->setGamePad(0, m_padLeft.get());
    m_bus->setGamePad(1, m_padRight.get());

    try
    {
        Log::i("Loading ROM file %s", romFileName);
        ROMLoader loader { *m_cartridge };
        loader.loadNES(romFileName);
        m_bus->injectCartrige(m_cartridge.get());
    }
    catch (const Exception &ex)
    {
        Log::e("Failed to load ROM file: %s", ex.message());
    }
}

void MainWindow::update()
{
    if (m_bus->getCartrige())
    {
        m_bus->runFrame();
    }
    else
    {
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

void MainWindow::handleEvent(const SDL_Event &evt)
{
    if (evt.type == SDL_KEYUP || evt.type == SDL_KEYDOWN)
    {
        const auto key = evt.key.keysym.scancode;
        const bool pressed = evt.key.state == SDL_PRESSED;
        auto i = std::find_if(std::begin(m_keyMapLeft),
                              std::end(m_keyMapLeft),
                              [key](const KeyMap &x)
        {
            return x.sdlKey == key;
        });

        if (i != std::end(m_keyMapLeft))
            m_padLeft->buttonEvent(i->padKey, pressed, i->turbo, false);
        else
        {
            // Pad 2?
            i = std::find_if(std::begin(m_keyMapRight),
                             std::end(m_keyMapRight),
                             [key](const KeyMap &x)
            {
                return x.sdlKey == key;
            });

            if (i != std::end(m_keyMapRight))
                m_padRight->buttonEvent(i->padKey, pressed, i->turbo, false);
        }
    }
}
