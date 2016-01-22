#include <thread>
#include "cpu6502.h"
#include "Cartridge.h"
#include "debugger.h"


int main()
{
    Cartrige cartrige;
    cartrige.rom[1].Write(0x1ffd, 0xc0);
    cartrige.rom[1].Write(0x1ffc, 0x00);


/*
LDX #$4c
STX $0000
LDX #$10
STX $0001
LDX #$c0
STX $0002

;now $0000  contains 4c 10 c0 which means JMP c010

LDA #1

ADC #$11
JMP $0000
*/
    cartrige.rom[1].Write(0x0,0xa2);
    cartrige.rom[1].Write(0x1,0x4c);
    cartrige.rom[1].Write(0x2,0x8e);
    cartrige.rom[1].Write(0x3,0x00);
    cartrige.rom[1].Write(0x4,0x00);
    cartrige.rom[1].Write(0x5,0xa2);
    cartrige.rom[1].Write(0x6,0x10);
    cartrige.rom[1].Write(0x7,0x8e);
    cartrige.rom[1].Write(0x8,0x01);
    cartrige.rom[1].Write(0x9,0x00);
    cartrige.rom[1].Write(0xa,0xa2);
    cartrige.rom[1].Write(0xb,0xc0);
    cartrige.rom[1].Write(0xc,0x8e);
    cartrige.rom[1].Write(0xd,0x02);
    cartrige.rom[1].Write(0xe,0x00);
    cartrige.rom[1].Write(0xf,0xa9);
    cartrige.rom[1].Write(0x10,0x01);
    cartrige.rom[1].Write(0x11,0x69);
    cartrige.rom[1].Write(0x12,0x11);
    cartrige.rom[1].Write(0x13,0x4c);
    cartrige.rom[1].Write(0x14,0x00);
    cartrige.rom[1].Write(0x15,0x00);


    CPU6502 cpu(CPU6502::NTSC);
    cpu.InjectCartrige(&cartrige);
    Debugger dbg(&cpu);
    dbg.Start(10000);
    while (1)
        std::this_thread::yield();
}