#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "sdl_playback_be.h"
#include <bus.h>
#include <PPU.h>
#include <APU.h>
#include <cpu6502.h>
#include <gamepad.h>
#include <Cartridge.h>
#include <SDL2/SDL.h>

#ifdef USE_VULKAN
    #include <SDL2/SDL_vulkan.h>
    #include "vkrbe.h"
#else
    #include "glfuncwrp.h"
    #include "glbe.h"
#endif

#include <string>

class MainWindow
{
    struct KeyMap
    {
        SDL_Scancode sdlKey;
        Button padKey;
        bool turbo;
    };

    SDL_Window *m_sdlWin = nullptr;

    Bus m_bus { OutputMode::NTSC };
    CPU6502 m_cpu;
    PPU m_ppu;
    APU m_apu;
    Cartrige m_cartridge;
    Gamepad m_padLeft,
            m_padRight;
#ifdef USE_VULKAN
    VulkanRenderingBackend m_RBE;
#else
    SDL_GLContext m_glCtx = { };
    GLFunctionsWrapper m_glFuncWrp;
    GLRenderingBackend<GLFunctionsWrapper> m_RBE;
#endif
    SDLPlaybackBackend m_audioBE;
    bool m_isPaused = false,
         m_doStep = false;

    KeyMap m_keyMapLeft[10] = {
         { SDL_SCANCODE_W,   Button::UP,     false },
         { SDL_SCANCODE_S,   Button::DOWN,   false },
         { SDL_SCANCODE_A,   Button::LEFT,   false },
         { SDL_SCANCODE_D,   Button::RIGHT,  false },
         { SDL_SCANCODE_1,   Button::START,  false },
         { SDL_SCANCODE_2,   Button::SELECT, false },
         { SDL_SCANCODE_K,   Button::A,      false },
         { SDL_SCANCODE_L,   Button::B,      false },
         { SDL_SCANCODE_I,   Button::A,      true },
         { SDL_SCANCODE_O,   Button::B,      true }
     };

     KeyMap m_keyMapRight[8] = {
         { SDL_SCANCODE_UP,       Button::UP,     false },
         { SDL_SCANCODE_DOWN,     Button::DOWN,   false },
         { SDL_SCANCODE_LEFT,     Button::LEFT,   false },
         { SDL_SCANCODE_RIGHT,    Button::RIGHT,  false },
         { SDL_SCANCODE_9,        Button::START,  false },
         { SDL_SCANCODE_0,        Button::SELECT, false },
         { SDL_SCANCODE_PAGEUP,   Button::A,      false },
         { SDL_SCANCODE_PAGEDOWN, Button::B,      false }
     };

     std::string m_error;

#ifdef USE_IMGUI
     void handleUI();
#endif

public:
#ifdef USE_VULKAN
    MainWindow(SDL_Window *win);
#else
    MainWindow(SDL_Window *win, SDL_GLContext glCtx);
#endif

    ~MainWindow();
    void initialize();
    void loadROM(const char *romFileName);
    void update();
    void handleEvent(const SDL_Event &evt);

    int getRefreshRate() const noexcept
    {
        return m_bus.getMode() == OutputMode::NTSC ? 60 : 50;
    }
};

#endif
