#include "PPU.h"
#include "log.h"


c6502_byte_t PPU::readRegister(c6502_word_t n) const noexcept
{
    Log::v("Reading PPU register #%d", n);
    if (n == 2)
    {
        return 0b10000000u;
    }
    return m_registers.Read(n);
}

void PPU::writeRegister(c6502_word_t n, c6502_byte_t val) noexcept
{
    Log::v("Writing value %d to PPU register #%d", val, n);
    m_registers.Write(n, val);
}

void PPU::clock()
{
    // Prepare state...
    m_bus.generateNMI();
}
