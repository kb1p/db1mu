#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "glfuncwrp.h"
#include "glbe.h"
#include <bus.h>
#include <PPU.h>
#include <cpu6502.h>
#include <gamepad.h>
#include <Cartridge.h>
#include <SDL2/SDL.h>
#include <memory>

class MainWindow
{
    struct KeyMap
    {
        SDL_Scancode sdlKey;
        Button padKey;
        bool turbo;
    };

    using RenderingBackend = GLRenderingBackend<GLFunctionsWrapper>;

    SDL_Window *m_sdlWin = nullptr;
    std::unique_ptr<Bus> m_bus;
    std::unique_ptr<CPU6502> m_cpu;
    std::unique_ptr<PPU> m_ppu;
    std::unique_ptr<Cartrige> m_cartridge;
    std::unique_ptr<Gamepad> m_padLeft,
                             m_padRight;
    GLFunctionsWrapper m_glFuncWrp;
    std::unique_ptr<RenderingBackend> m_renderingBackend;
    bool ready = false;

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

     void handleUI();

public:
    MainWindow(SDL_Window *win, SDL_GLContext glCtx);
    ~MainWindow();
    void initialize();
    void loadROM(const char *romFileName);
    void update();
    void handleEvent(const SDL_Event &evt);
};

#endif
