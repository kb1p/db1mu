#include "mainwindow.h"

struct Options
{
    const char *romFileName;
    bool fullScreen;
};

Options parseArguments(int argc, char *argv[])
{
    Options opts = {
        nullptr,
        false
    };

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--fullscreen") == 0)
            opts.fullScreen = true;
        else if (argv[i][0] != '-')
            opts.romFileName = argv[i];
        else
            throw "unrecognized command line option";
    }

#ifndef USE_IMGUI
    if (opts.romFileName == nullptr)
        throw "ROM file name to load was not provided";
#endif

    return opts;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    SDL_Window *win = { };
    SDL_GLContext glCtx = { };
    try
    {
        auto opts = parseArguments(argc - 1, argv + 1);

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            throw "SDL initialization failed";

        // Set to use OpenGL ES 2.0
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

        win = SDL_CreateWindow("db1mu",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               640,
                               480,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (!win)
            throw "failed to create SDL window";

        if (opts.fullScreen)
            SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);

        glCtx = SDL_GL_CreateContext(win);
        if (!glCtx)
            throw "failed to create GLES context";

        const bool manualDelay = SDL_GL_SetSwapInterval(1) < 0;

        {
            MainWindow emuWin { win, glCtx };

            // Rendering state setup
            emuWin.initialize();
            if (opts.romFileName)
                emuWin.loadROM(opts.romFileName);

            bool runLoop = true;
            while (runLoop)
            {
                const auto t0 = SDL_GetTicks();
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
                const auto dt = SDL_GetTicks() - t0;
                if (manualDelay && dt < 16)
                    SDL_Delay(16 - dt);
            }
        }
    }
    catch (const char *msg)
    {
        std::string fullMsg = "Initialization error: ";
        fullMsg += msg;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "Error",
                                 fullMsg.c_str(),
                                 win);
        rc = 1;
    }

    if (glCtx)
        SDL_GL_DeleteContext(glCtx);
    if (win)
        SDL_DestroyWindow(win);
    SDL_Quit();

    return rc;
}
