#ifndef __MMC1_H__
#define __MMC1_H__

#include "Cartridge.h"

/*
 * Chip & Dale 1, Choujin Sentain Jetman, Clash at the Demonhead...
 */
class MMC1: public Mapper
{
    uint m_shiftReg = 0u,
         m_modeChr = 0u,
         m_modePrg = 0u;
    int m_curChr[2] = { 0, 0 },
        m_curPrg = 0,
        m_nWrites = 0;
    Maybe<Mirroring> m_mirrOverride;

    void writeRegister(c6502_word_t addr, c6502_byte_t val);

public:
    using Mapper::Mapper;

    c6502_byte_t readROM(c6502_word_t addr) override;

    c6502_byte_t readRAM(c6502_word_t addr) override;

    c6502_byte_t readVROM(c6502_word_t addr) override;

    void writeRAM(c6502_word_t addr, c6502_byte_t val) override;

    Mirroring updateMirroring(Mirroring cur) noexcept override;
};

#endif
