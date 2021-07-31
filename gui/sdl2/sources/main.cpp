#include "mainwindow.h"
#include <iostream>

int main(int argc, char **argv)
{
    try
    {
        if (argc < 2)
            throw "ROM file name to load was not provided";

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            throw "SDL initialization failed";

        // Set to use OpenGL ES 2.0
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

        auto win = SDL_CreateWindow("db1mu",
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    640,
                                    480,
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        if (!win)
            throw "failed to create SDL window";

        auto glCtx = SDL_GL_CreateContext(win);
        if (!glCtx)
            throw "failed to create GLES context";

        if (SDL_GL_SetSwapInterval(1) < 0)
            throw "failed to set VSync interval";

        MainWindow emuWin;

        // Rendering state setup
        emuWin.initialize(argv[1]);

        bool runLoop = true;
        while (runLoop)
        {
            SDL_Event evt;
            while (SDL_PollEvent(&evt) != 0)
            {
                if (evt.type == SDL_QUIT)
                    runLoop = false;
                else
                    emuWin.handleEvent(evt);
            }

            // Update emulator state and render scene with GL
            emuWin.update();

            SDL_GL_SwapWindow(win);
        }

        SDL_GL_DeleteContext(glCtx);
        SDL_DestroyWindow(win);
        SDL_Quit();
    }
    catch (const char *msg)
    {
        std::cerr << "*** Error: " << msg << std::endl;
        return 1;
    }

    return 0;
}
