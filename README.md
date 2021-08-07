# db1mu
A cross-platform NES emulator.

## Description
This is a NES emulator project that is aimed to be highly portable. The final goal is to be able to launch it on smart TV platforms. The emulation core is written completely in stadard C++; for communication with the platform-specific functions (rendering, sound), the core provides interfaces that must be implemented by frontends. Currently 2 frontends implemented: Qt5 and SDL.

## What's already done
* CPU, PPU, gamepad modules.
* Default mapper which supports 1-2 16kb ROM banks and 1 8kb VROM bank.
* Frontends: Qt5, SDL2 (with optional [ImGui](https://github.com/Flix01/imgui) interface).

## To do
* Implement APU emulation.
* Extend the set of supported mappers.
* Implement undocumented CPU instructions support.

## Build & run
### Prerequisites
* C++ compiler with C++11 standard support (tested on GCC, MinGW).
* CMake 3.1 or higher.
* Flex / Bison (for debugger).
* For frontend: [Qt5](https://www.qt.io/download) or [SDL2](https://libsdl.org/download-2.0.php).
* GLES 2.0 emulator for desktop platforms ([ARM Mali GLES emulator](https://developer.arm.com/tools-and-software/graphics-and-gaming/opengl-es-emulator/downloads) or [ANGLE](https://github.com/google/angle)).

### Qt5 frontend
```bash
$ mkdir build ; cd build
$ cmake -DCMAKE_INSTALL_PREFIX=path/to/install -DCMAKE_BUILD_TYPE=Release -DFRONTEND_TYPE=QT ..
$ cmake --build . --config release --target install
```
Now just launch `path/to/install/bin/b1mulator` executable.

#### Windows platform
To create standalone executable, use `windeployqt` utility as follows:
```bash
$ cd path/to/install/bin
$ windeployqt --no-patchqt --release b1mulator
```

### SDL frontend
```bash
$ mkdir build ; cd build
$ cmake -DCMAKE_INSTALL_PREFIX=path/to/install -DCMAKE_BUILD_TYPE=Release -DFRONTEND_TYPE=SDL ..
$ cmake --build . --config release --target install
```
To disable InGui, add `-DUSE_IMGUI=OFF` command to configuration line. In this case ROM file has to be provided as command line argumant, switching between ROMs during application runtime is not possible.

#### Windows platform
- Besides [SDL2](https://libsdl.org/download-2.0.php) some GLES 2+ emulation libraries (e.g. [Mali GLES emulator](https://developer.arm.com/tools-and-software/graphics-and-gaming/opengl-es-emulator/downloads)) need to be installed.
- If paths to GLES headers / libraries cannot be figured out using `KHRONOS_HEADERS` and `OPENGLES_LIBDIR` environment variables, need to provide these paths to cmake configuration by setting `-DGLES_HDR_PATH=...` and `-DGLES_LIB_PATH=...` variables.
- To create standalone executable, manually copy SDL2.dll, GLESv2.dll, etc. to `path/to/install/bin`.