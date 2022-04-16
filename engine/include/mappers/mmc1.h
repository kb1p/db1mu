#ifndef __MMC1_H__
#define __MMC1_H__

#include "Cartridge.h"

/*
 * Chip & Dale 1, Choujin Sentain Jetman, Clash at the Demonhead...
 */
class MMC1: public Mapper
{
    uint m_shiftReg = 0u;

public:
    using Mapper::Mapper;

    c6502_byte_t readROM(c6502_word_t addr) override;

    c6502_byte_t readRAM(c6502_word_t addr) override;

    c6502_byte_t readVROM(c6502_word_t addr) override;

    void writeRAM(c6502_word_t addr, c6502_byte_t val) override;
};

#endif
