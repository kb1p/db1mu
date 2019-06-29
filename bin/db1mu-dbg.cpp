#include <thread>
#include "cpu6502.h"
#include "PPU.h"
#include "Cartridge.h"
#include "debugger.h"
#include "loader.h"
#include "log.h"
#include <iostream>
#include <fstream>

int main()
{
    std::ofstream logFile { "log.txt", std::ios::app };
    auto &logCfg = Log::instance().config();
    logCfg.pOutput = &logFile;
    logCfg.filter = Log::LEVEL_VERBOSE;
    logCfg.autoFlush = true;

    Bus systemBus;
    CPU6502 cpu { CPU6502::NTSC, systemBus };
    systemBus.setCPU(&cpu);
    PPU ppu { systemBus, nullptr };
    systemBus.setPPU(&ppu);
    Cartrige cartrige;
    std::ifstream in("raw.data", std::ios::in | std::ios::binary);
    ROMLoader loader(cartrige);
    try
    {
        // Find the "raw.data" sample file in the "test" folder
        //loader.loadRawData(in, 0xC000, Mapper::ROM_SIZE);
        // Try and see something like this:
        loader.loadNES("smb.nes");
    }
    catch (const Exception &ex)
    {
        in.close();
        std::cerr << "Shit happens: " << ex.message() << std::endl;
        return 1;
    }
    in.close();

    systemBus.injectCartrige(&cartrige);
    Debugger dbg { systemBus };
    dbg.Start(10000);
    while (1)
        std::this_thread::yield();;
}
