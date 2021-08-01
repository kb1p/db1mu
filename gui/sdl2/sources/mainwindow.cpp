#include "mainwindow.h"

#include <log.h>
#include <loader.h>
#include <algorithm>
#include <iostream>

#ifdef USE_IMGUI
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include <imguifilesystem/imguifilesystem.h>
#endif

MainWindow::MainWindow(SDL_Window *win, SDL_GLContext glCtx):
    m_sdlWin { win }
{
#ifdef USE_IMGUI
    // ImGUI initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(win, glCtx);
    ImGui_ImplOpenGL3_Init();
    ImGui::GetIO();
#endif
}

MainWindow::~MainWindow()
{
#ifdef USE_IMGUI
    // ImGUI initialization
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
#endif
}

void MainWindow::initialize()
{
    auto &logCfg = Log::instance().config();
    logCfg.pOutput = &std::cout;
    logCfg.filter = Log::LEVEL_DEBUG;
    logCfg.autoFlush = true;

    m_renderingBackend.reset(new GLRenderingBackend<GLFunctionsWrapper>);
    m_renderingBackend->init(&m_glFuncWrp);
    int w = 0, h = 0;
    SDL_GetWindowSize(m_sdlWin, &w, &h);
    m_renderingBackend->resize(w, h);

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
}

void MainWindow::loadROM(const char *romFileName)
{
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

    handleUI();
}

void MainWindow::handleEvent(const SDL_Event &evt)
{
#ifdef USE_IMGUI
    ImGui_ImplSDL2_ProcessEvent(&evt);
#endif

    switch (evt.type)
    {
        case SDL_KEYUP:
        case SDL_KEYDOWN:
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
            break;
        case SDL_WINDOWEVENT:
            if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                m_renderingBackend->resize(evt.window.data1, evt.window.data2);
    }
}

void MainWindow::handleUI()
{
#ifdef USE_IMGUI
    // Draw ImGui stuff
    auto &imGuiIO = ImGui::GetIO();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(m_sdlWin);
    ImGui::NewFrame();

    // Render actions
    bool cmdOpenROM = false,
         cmdQuit = false;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            cmdOpenROM = ImGui::MenuItem("Open ROM");
            ImGui::Separator();
            cmdQuit = ImGui::MenuItem("Exit");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Process UI actions
    static ImGuiFs::Dialog dlg;
    const char *romPath = dlg.chooseFileDialog(cmdOpenROM);
    if (strlen(romPath) > 0)
        loadROM(romPath);

    if (cmdQuit)
    {
        SDL_Event evt;
        evt.type = SDL_QUIT;
        SDL_PushEvent(&evt);
    }

    ImGui::Render();
    glViewport(0, 0, (int)imGuiIO.DisplaySize.x, (int)imGuiIO.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}
