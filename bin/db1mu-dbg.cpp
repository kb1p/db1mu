#include <thread>
#include "cpu6502.h"
#include "Cartridge.h"
#include "debugger.h"


int main()
{
    Cartrige cartrige;
    cartrige.rom[1].Write(0x1ffd, 0xff);
    cartrige.rom[1].Write(0x1ffc, 0x00);

    cartrige.rom[1].Write(0x1f00, 0x69); // ADC IMM
    cartrige.rom[1].Write(0x1f01, 0x55); // operand
    cartrige.rom[1].Write(0x1f02, 0x4c); // JMP ABS
    cartrige.rom[1].Write(0x1f03, 0x00); // operand
    cartrige.rom[1].Write(0x1f04, 0xff); // operand

    CPU6502 cpu(CPU6502::NTSC);
    cpu.InjectCartrige(&cartrige);
    Debugger dbg(&cpu);
    dbg.Start(10000);
    while (1)
        std::this_thread::yield();
}