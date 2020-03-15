# db1mu
A cross-platform NES emulator.

## Description
This is a NES emulator project that is aimed to be highly portable. The final goal is to be able to launch it on smart TV platforms. The emulation core is written completely in stadard C++; for communication with the platform-specific functions (rendering, sound), the core provides interfaces that must be implemented by frontends. For debug and testing purposes, a reference Qt5-based frontend implementation is provided, although for most of the smart TV target platforms SDL-based frontend would be required.  
_The project is still under development._

## What's already done
* CPU, PPU, gamepad modules.
* Initial version of reference frontend using Qt5. _Tested on Ubuntu 18.04 and Raspbian 10._
* Default mapper which supports 1-2 16kb ROM banks and 1 8kb VROM bank.

## To do
* Implement APU emulation.
* Extend the set of supported mappers.
* Optimize GLESv2 rendering backend.
* Implement SDL frontend to simplify porting.

## Build & run
_Currently only Qt5 frontend is available._

#### Prerequisites
* C++ compiler with C++11 standard support. _Tested with GCC._
* CMake 3.1 or higher.
* Flex / Bison (for debugger).
* Qt5 (for Qt5 frontend).

#### Qt5-based frontend
```
$ mkdir build ; cd build
$ cmake ..
$ cmake --build .
$ gui/b1mulator
```
