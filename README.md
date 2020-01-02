# db1mu
A cross-platform NES emulator.

## Description
This is a NES emulator development project that is aimed to be highly portable. The final goal is to be able to launch it on smart TV platforms. The emulation core is written completely in stadard C++; for communication with the platform-specific functions (rendering, sound), the core provides interfaces that must be implemented by frontends. For debug and testing purposes, a reference Qt5-based frontend implementation is provided, although for most of the smart TV target platforms SDL-based frontend would be required.  
_The project is still under development._

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

## ToDo
* Implement APU emulation.
* Implement commonly used mappers _(currently only the default mapper allowing at most 2 ROM banks + 1 VROM bank is supported)_.
* Optimize rendering.
* Implement SDL frontend to simplify porting.