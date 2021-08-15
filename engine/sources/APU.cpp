#include "APU.h"
#include "log.h"

c6502_byte_t APU::readRegister(c6502_word_t reg)
{
    c6502_byte_t rv = 0;
    switch (reg)
    {
        case CTRL_STATUS:
            rv = 0;
            break;
        default:
            Log::e("Attempt to read from illegal APU register 0x%X (returned zero)", reg);
            assert(false);
    }
    return rv;
}

void APU::writeRegister(c6502_word_t reg, c6502_byte_t val)
{
}
