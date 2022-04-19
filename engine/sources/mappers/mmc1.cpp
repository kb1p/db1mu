#include "mappers/mmc1.h"

MMC1::MMC1(int nROMs, int nVROMs, int nRAMs):
    Mapper { nROMs, nVROMs, nRAMs }
{
    setFeature<RAM>(nRAMs > 0);
}

c6502_byte_t MMC1::readMem(c6502_word_t addr)
{
    if (addr >= 0xC000u)
    {
        auto &bh = romBank(m_modePrg == 3u ? numROMs() - 1  :
                           m_modePrg == 2u ? m_curPrg       :
                           m_curPrg + 1);
        return bh.Read(addr - 0xC000u);
    }
    else if (addr >= 0x8000u)
    {
        auto &bl = romBank(m_modePrg == 2u ? 0 :
                           m_curPrg);
        return bl.Read(addr - 0x8000u);
    }
    else if (addr >= 0x6000u && numRAMs() >= 1)
    {
        return ramBank(0).Read(addr - 0x6000u);
    }
    else
        throw Exception(Exception::IllegalArgument,
                        "illegal mapper memory address");
}

c6502_byte_t MMC1::readVideoMem(c6502_word_t addr)
{
    if (addr >= 0x2000u)
        throw Exception { Exception::IllegalArgument,
                         "illegal VROM address" };

    auto off = addr;
    auto ind = m_curChr[0];
    if (m_modeChr == 1u)
    {
        // 4K CHR bank mode. We store CHR ROMs in 8K banks so need
        // to calculate bank index + offset
        if (addr < 0x1000u)
        {
            ind = m_curChr[0] / 2;
            if (m_curChr[0] % 2 == 1)
                off += 0x1000u;
        }
        else
        {
            ind = m_curChr[1] / 2;
            if (m_curChr[1] % 2 == 0)
                off -= 0x1000u;
        }
    }

    return vromBank(ind).Read(off);
}

Mirroring MMC1::updateMirroring(Mirroring cur) noexcept
{
    return m_mirrOverride.value(cur);
}

void MMC1::writeMem(c6502_word_t addr, c6502_byte_t val)
{
    if (addr >= 0x6000u && addr < 0x8000u)
    {
        if (numRAMs() < 1)
            throw Exception { Exception::IllegalOperation,
                              "this MMC1 controller has no RAM" };
        else
            ramBank(0).Write(addr - 0x6000u, val);
    }
    else if (addr >= 0x8000u)
    {
        // Write to MMC1 register
        if (val & 0x80u)
        {
            m_shiftReg = 0x10u;

            // Lock last 16kB ROM bank to 0xC000
            m_modePrg = 3;
        }
        else
        {
            // Check if shift register is full
            const auto pv = (val << 4u) & 0x10u;
            if (m_shiftReg & 0x01u)
            {
                writeRegister(addr, pv | (m_shiftReg >> 1u));
                m_shiftReg = 0x10u;
            }
            else
                m_shiftReg = pv | (m_shiftReg >> 1u);
        }
    }
    else
        throw Exception { Exception::IllegalOperation,
                          "incorrect write to MMC1 register" };
}

void MMC1::writeRegister(c6502_word_t addr, c6502_byte_t val)
{
    if (addr < 0xA000u)
    {
        // Control
        switch (val & 0b11u)
        {
            case 0u:
                m_mirrOverride = Mirroring::SingleLower;
                break;
            case 1u:
                m_mirrOverride = Mirroring::SingleUpper;
                break;
            case 2u:
                m_mirrOverride = Mirroring::Vertical;
                break;
            case 3u:
                m_mirrOverride = Mirroring::Horizontal;
                break;
        }
        m_modePrg = (val >> 2u) & 0b11u;
        m_modeChr = (val >> 4u) & 0x01u;
    }
    else if (addr < 0xC000u)
        // CHR0 bank selector
        m_curChr[0] = val & (m_modeChr == 0u ? 0x1Eu : 0x1Fu);
    else if (addr < 0xE000u)
        // CHR1 bank selector
        m_curChr[1] = val & 0x1Fu;
    else
    {
        // PRG bank
        m_curPrg = val & (m_modePrg > 1u ? 0x0Fu : 0x0Eu);

        // TODO: MMC1A, MMC1B logic
    }
}
