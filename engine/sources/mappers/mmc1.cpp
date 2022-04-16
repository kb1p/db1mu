#include "mappers/mmc1.h"

c6502_byte_t MMC1::readROM(c6502_word_t addr)
{
    return 0;
}

c6502_byte_t MMC1::readRAM(c6502_word_t addr)
{
    return 0;
}

c6502_byte_t MMC1::readVROM(c6502_word_t addr)
{
    return 0;
}

void MMC1::writeRAM(c6502_word_t addr, c6502_byte_t val)
{
    if (addr >= 0x6000u && addr <= 0x7FFFu)
    {
        if (m_nRAMs < 1)
            throw Exception(Exception::IllegalOperation,
                            "default mapper has no RAM");
    }
}
