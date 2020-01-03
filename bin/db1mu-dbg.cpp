#include <thread>
#include "cpu6502.h"
#include "PPU.h"
#include "Cartridge.h"
#include "debugger.h"
#include "loader.h"
#include "log.h"
#include <iostream>
#include <fstream>

class DummyBackend: public PPU::RenderingBackend
{
public:
    void setBackground(c6502_byte_t color) override
    {
    }

    void setSymbol(Layer l, int x, int y, c6502_byte_t colorData[64]) override
    {
    }

    void draw() override
    {
    }
};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << "<ROM-file> [<log-file>]" << std::endl;
        return 1;
    }

    std::ofstream logFile { argc > 2 ? argv[2] : "log.txt", std::ios::app };
    auto &logCfg = Log::instance().config();
    logCfg.pOutput = &logFile;
    logCfg.filter = Log::LEVEL_VERBOSE;
    logCfg.autoFlush = true;

    Bus systemBus { OutputMode::NTSC };
    CPU6502 cpu;
    systemBus.setCPU(&cpu);
    DummyBackend db;
    PPU ppu { &db };
    systemBus.setPPU(&ppu);
    Cartrige cartrige;
    ROMLoader loader(cartrige);
    try
    {
        loader.loadNES(argv[1]);
    }
    catch (const Exception &ex)
    {
        std::cerr << "Error: " << ex.message() << std::endl;
        return 1;
    }

    systemBus.injectCartrige(&cartrige);
    Debugger dbg { systemBus };
    dbg.Start(10000);
    while (1)
        std::this_thread::yield();;
}
