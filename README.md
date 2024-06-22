# db1mu
A cross-platform NES emulator.

## Description
This is a NES emulator project that is aimed to be highly portable. The final goal is to be able to launch it on smart TV platforms. The emulation core is written completely in stadard C++; for communication with the platform-specific functions (rendering, sound), the core provides interfaces that must be implemented by frontends. Currently 2 frontends implemented: Qt5 and SDL.

## What's already done
* CPU, PPU, gamepad modules.
* Default mapper which supports 1-2 16kb ROM banks and 1 8kb VROM bank.
* Frontends: Qt5, SDL2 (with optional [ImGui Addons](https://github.com/Flix01/imgui) interface).
* Backends (rendering): OpenGL ES 2.0, Vulkan

## To do
* Extend the set of supported mappers.
* Implement undocumented CPU instructions support.

## Build & run
### Prerequisites
* C++ compiler with C++11 standard support (tested on GCC, MinGW).
* CMake 3.1 or higher.
* Flex / Bison (for debugger).
* For frontend: [Qt5](https://www.qt.io/download) or [SDL2](https://libsdl.org/download-2.0.php).
* For OpenGL ES 2.0 rendering: emulator for desktop platforms ([ARM Mali GLES emulator](https://developer.arm.com/tools-and-software/graphics-and-gaming/opengl-es-emulator/downloads) or [ANGLE](https://github.com/google/angle)).
* For Vulkan: Vulkan SDK, glslValidator

### Qt5 frontend
#### Install required Qt5 components
```bash
$ sudo apt install qt5-default qtmultimedia5-dev libqt5multimedia5-plugins 
```

#### Build the emulator itself
```bash
$ mkdir build ; cd build
$ cmake -DCMAKE_INSTALL_PREFIX=path/to/install -DCMAKE_BUILD_TYPE=Release -DFRONTEND_TYPE=QT ..
$ cmake --build . --config release --target install
```
Now just launch `path/to/install/bin/b1mulator` executable.

#### Notes for Windows platform
To create standalone executable, use `windeployqt` utility as follows:
```bash
$ cd path/to/install/bin
$ windeployqt --no-patchqt --release b1mulator
```

#### Running emulator with Qt5 frontend
The following command line options are available:

Option             | Effect
-------------------|---------
`path/to/rom.file` | Load and run iNES ROM file immediatelly at startup.

### SDL frontend
SDL frontend can be built with or without UI based on *ImGui Addons* UI.

#### Install SDL2
Installing `dev` package for SDL2 should be enough:
```bash
$ sudo apt install libsdl2-dev
```

#### Build emulator with ImGui Addons UI
UI is developed using *ImGui Addons* project (https://github.com/Flix01/imgui). It is declared as submodule in `gui/sdl2/third_party` directory and needs to be checked out before building the emulator:
```bash
$ git submodule init
$ git submodule update
```
After checking out *ImGui Addons* source code is done, use the following commands to build emulator frontend:
```bash
$ mkdir build ; cd build
$ cmake -DCMAKE_INSTALL_PREFIX=path/to/install -DCMAKE_BUILD_TYPE=Release -DFRONTEND_TYPE=SDL ..
$ cmake --build . --config release --target install
```

#### Build emulator without UI
Use the following commands:
```bash
$ mkdir build ; cd build
$ cmake -DCMAKE_INSTALL_PREFIX=path/to/install -DCMAKE_BUILD_TYPE=Release -DFRONTEND_TYPE=SDL -DUSE_IMGUI=OFF ..
$ cmake --build . --config release --target install
```
If UI is not used, there is no way to pick ROM file interactively, so a path to ROM file **must** be provided as command line argument, e.g. `$ b1mulator path/to/rom.nes`. If it is not provided, the program will output error message and exit.

#### Notes for Windows platform
- Besides [SDL2](https://libsdl.org/download-2.0.php) some GLES 2+ emulation libraries (e.g. [Mali GLES emulator](https://developer.arm.com/tools-and-software/graphics-and-gaming/opengl-es-emulator/downloads)) need to be installed.
- If paths to GLES headers / libraries cannot be figured out using `KHRONOS_HEADERS` and `OPENGLES_LIBDIR` environment variables, need to provide these paths to cmake configuration by setting `-DGLES_HDR_PATH=...` and `-DGLES_LIB_PATH=...` variables.
- To create standalone executable, manually copy SDL2.dll, GLESv2.dll, etc. to `path/to/install/bin`.

#### Running emulator with SDL frontend
Command line options can be specified in any order. The following command line options are available:
Option             | Effect
-------------------|---------
`--fullscreen`     | Run in fullscreen mode (if not specified, run in windowed mode)
`path/to/rom.file` | Load and run iNES ROM file immediatelly at startup (mandatory if UI is not used).

### Switching to Vulkan renderer
By default, GLES renderer is used. To use Vulkan renderer (for either Qt and SDL frontend) follow below steps:

- Make sure to install **Vulkan SDK** and **glslangValidator** to compile GLSL to SPIR-V:  
  ```bash
  $ sudo apt install libvulkan-dev vulkan-validationlayers-dev mesa-vulkan-drivers vulkan-tools glslang-tools
  ```
- Add `RENDERER_TYPE=Vulkan` to cmake configuration line like in example below:  
  ```bash
  $ cmake -DCMAKE_BUILD_TYPE=Debug -DRENDERER_TYPE=Vulkan ..
  $ cmake --build . --config debug
  ```

Please note that **Vulkan validation layers** are enabled in debug build configurations, in release configurations they are disabled. Validation layer output is printed to db1mu log system for SDL2, to default Qt's debug output for Qt5.
