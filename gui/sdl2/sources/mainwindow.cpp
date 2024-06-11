#include "mainwindow.h"

#include <log.h>
#include <loader.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <stdexcept>

#ifdef USE_IMGUI
#include <imgui.h>
    #ifdef USE_VULKAN
        #include <imgui_impl_vulkan.h>
    #else
        #include <imgui_impl_opengl3.h>
    #endif
#include <imgui_impl_sdl.h>
#include <imguifilesystem/imguifilesystem.h>
#endif

using std::runtime_error;

#if defined(USE_VULKAN) && defined(USE_IMGUI)
static void checkVkResult(VkResult err)
{
    if (err == 0)
        return;

    Log::e("[Vulkan][ImGUI] Error: VkResult = %d", err);

    if (err < 0)
        throw runtime_error { "ImGUI Vulkan backend error" };
}
#endif

#ifdef USE_VULKAN
MainWindow::MainWindow(SDL_Window *win):
    m_sdlWin { win }
#else
MainWindow::MainWindow(SDL_Window *win, SDL_GLContext glCtx):
    m_sdlWin { win },
    m_glCtx { glCtx }
#endif
{
}

MainWindow::~MainWindow()
{
#ifdef USE_IMGUI
    #ifdef USE_VULKAN
        m_RBE.waitDeviceIdle();
        ImGui_ImplVulkan_Shutdown();
    #else
        ImGui_ImplOpenGL3_Shutdown();
    #endif
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

#ifdef USE_VULKAN
    unsigned int extensionCount = 0;
    std::vector<const char*> reqExts;
    SDL_Vulkan_GetInstanceExtensions(m_sdlWin, &extensionCount, nullptr);
    reqExts.resize(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(m_sdlWin, &extensionCount, reqExts.data());

    #ifndef NDEBUG
        reqExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    m_RBE.init(reqExts.size(), reqExts.data());

    VkSurfaceKHR surf;
    if (!SDL_Vulkan_CreateSurface(m_sdlWin, m_RBE.instance(), &surf))
        throw runtime_error { "failed to create SDL Surface for Vulkan" };

    int winW = 0, winH = 0;
    SDL_Vulkan_GetDrawableSize(m_sdlWin, &winW, &winH);
    m_RBE.resize(winW, winH);

    // Rendering state setup
    m_RBE.setupOutput(surf);
#else
    m_RBE.init(&m_glFuncWrp);
#endif

#ifdef USE_IMGUI
    // ImGUI initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    #ifdef USE_VULKAN
        ImGui_ImplSDL2_InitForVulkan(m_sdlWin);
        ImGui_ImplVulkan_InitInfo initInf = { };
        initInf.Instance = m_RBE.instance();
        initInf.PhysicalDevice = m_RBE.physicalDevice();
        initInf.Device = m_RBE.logicalDevice();
        initInf.QueueFamily = m_RBE.renderQueueFamilyIndex();
        initInf.Queue = m_RBE.renderQueue();
        initInf.PipelineCache = VK_NULL_HANDLE;
        initInf.DescriptorPool = m_RBE.descriptorPoolIG();
        initInf.Allocator = nullptr;
        initInf.MinImageCount = m_RBE.swcMinImageCount();
        initInf.ImageCount = m_RBE.swcImageCount();
        initInf.CheckVkResultFn = checkVkResult;
        ImGui_ImplVulkan_Init(&initInf, m_RBE.renderPassIG());

        auto cmdBuf = m_RBE.beginTransientCmdBufIG();
        ImGui_ImplVulkan_CreateFontsTexture(cmdBuf);
        m_RBE.endTransientCmdBufIG(cmdBuf);

        // To improve batching, we call ImGui rendering routine indirectly, from rendering backend
        // at the time of rendering of everything.
        m_RBE.setRenderFunctionIG([](VkCommandBuffer cmdBuf) noexcept
        {
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
        });
    #else
        ImGui_ImplSDL2_InitForOpenGL(m_sdlWin, m_glCtx);
        ImGui_ImplOpenGL3_Init();
    #endif
    ImGui::GetIO();
#endif

    m_ppu.setBackend(&m_RBE);

    m_apu.setBackend(&m_audioBE);

    m_bus.setCPU(&m_cpu);
    m_bus.setPPU(&m_ppu);
    m_bus.setAPU(&m_apu);
    m_bus.setGamePad(0, &m_padLeft);
    m_bus.setGamePad(1, &m_padRight);

    // Set initial viewport size
    int w = 0, h = 0;
    SDL_GetWindowSize(m_sdlWin, &w, &h);
    m_RBE.resize(w, h);
}

void MainWindow::loadROM(const char *romFileName)
{
    try
    {
        Log::i("Loading ROM file %s", romFileName);
        ROMLoader loader { m_cartridge };
        loader.loadNES(romFileName);
        m_bus.injectCartrige(&m_cartridge);
    }
    catch (const Exception &ex)
    {
        m_error = std::string{ "Failed to load ROM file, " } + ex.message();
        Log::e("%s", m_error.c_str());
    }
}

void MainWindow::update()
{
#ifdef USE_IMGUI
    handleUI();
#endif

    if (m_bus.getCartrige())
    {
        // If running emulation, perform a full frame iteration,
        // otherwise just repeat the last frame.
        if (!m_isPaused || m_doStep)
            m_bus.runFrame();
        else
            m_RBE.draw();
        m_doStep = false;
    }
    else
    {
        m_RBE.drawIdle();
    }

#if defined(USE_IMGUI) && !defined(USE_VULKAN)
    // For GLES, we just issue commands to paint ImGUI stuff over the main (emulator)
    // commands. For Vulkan, the paint command buffer will be filled indirectly from the
    // rendering backend itself.
    auto &imGuiIO = ImGui::GetIO();
    glViewport(0, 0, (int)imGuiIO.DisplaySize.x, (int)imGuiIO.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
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
            if ((evt.key.keysym.mod & KMOD_CTRL) && evt.key.state == SDL_PRESSED)
            {
                // Handle shortcuts
                switch (evt.key.keysym.scancode)
                {
                    case SDL_SCANCODE_P:
                        m_isPaused = !m_isPaused;
                        break;
                    case SDL_SCANCODE_S:
                        m_doStep = true;
                        break;
                }
            }
            else
            {
                // Handle gamepad mapped keys
                const auto key = evt.key.keysym.scancode;
                const bool pressed = evt.key.state == SDL_PRESSED;
                auto i = std::find_if(std::begin(m_keyMapLeft),
                                    std::end(m_keyMapLeft),
                                    [key](const KeyMap &x)
                {
                    return x.sdlKey == key;
                });

                if (i != std::end(m_keyMapLeft))
                    m_padLeft.buttonEvent(i->padKey, pressed, i->turbo, false);
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
                        m_padRight.buttonEvent(i->padKey, pressed, i->turbo, false);
                }
            }
            break;
        case SDL_WINDOWEVENT:
            if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
#if defined(USE_IMGUI) && defined(USE_VULKAN)
                ImGui_ImplVulkan_SetMinImageCount(m_RBE.swcMinImageCount());
#endif
                m_RBE.resize(evt.window.data1, evt.window.data2);
            }
    }
}

#ifdef USE_IMGUI
void MainWindow::handleUI()
{
    // Draw ImGui stuff
#ifdef USE_VULKAN
    ImGui_ImplVulkan_NewFrame();
#else
    ImGui_ImplOpenGL3_NewFrame();
#endif
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
        if (ImGui::BeginMenu("Emulation"))
        {
            ImGui::MenuItem("Pause", "Ctrl+P", &m_isPaused);
            if (ImGui::MenuItem("Step", "Ctrl+S"))
                m_doStep = true;
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Process UI actions
    static ImGuiFs::Dialog dlg;
    const char *romPath = dlg.chooseFileDialog(cmdOpenROM);
    if (strlen(romPath) > 0)
        loadROM(romPath);

    if (!m_error.empty())
        ImGui::OpenPopup("Error");

    if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s", m_error.c_str());
        ImGui::Separator();

        if (ImGui::Button("OK"))
        {
            m_error.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }

    if (cmdQuit)
    {
        SDL_Event evt;
        evt.type = SDL_QUIT;
        SDL_PushEvent(&evt);
    }

    ImGui::Render();
}
#endif
