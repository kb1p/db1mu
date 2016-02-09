#include <thread>
#include "cpu6502.h"
#include "Cartridge.h"
#include "debugger.h"
#include "loader.h"
#include <iostream>
#include <fstream>

int main()
{
    Cartrige cartrige;
    std::ifstream in("raw.data", std::ios::in | std::ios::binary);
    ROMLoader loader(cartrige);
    try
    {
        // Find the "raw.data" sample file in the "test" folder
        loader.loadRawData(in, 0xC000, Mapper::ROM_SIZE);
        // Try and see something like this:
        //loader.loadNES("smb.nes");
    }
    catch (const Exception &ex)
    {
        in.close();
        std::cerr << "Shit happens: " << ex.message() << std::endl;
        return 1;
    }
    in.close();

    CPU6502 cpu(CPU6502::NTSC);
    cpu.InjectCartrige(&cartrige);
    Debugger dbg(&cpu);
    dbg.Start(10000);
    while (1)
        std::this_thread::yield();
}