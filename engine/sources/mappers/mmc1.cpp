#include "mappers/mmc1.h"

c6502_byte_t MMC1::readROM(c6502_word_t addr)
{
    ROM_BANK *pBnkA = nullptr,
             *pBnkB = nullptr;
    switch (m_modePrg)
    {
        case 0u:
        case 1u:
            pBnkA = &romBank(m_curPrg & 0x0E);
            pBnkB = &romBank((m_curPrg & 0x0E) + 1);
            break;
        case 2u:
            pBnkA = &romBank(0);
            pBnkB = &romBank(m_curPrg);
            break;
        case 3u:
            pBnkA = &romBank(m_curPrg);
            pBnkB = &romBank(numROMs() - 1);
            break;
    }

    assert(pBnkA != nullptr && pBnkB != nullptr);

    // Switchable banks
    if (addr >= 0xC000u)
        return pBnkB->Read(addr - 0xC000u);
    else if (addr >= 0x8000u)
        return pBnkA->Read(addr - 0x8000u);
    else
        throw Exception(Exception::IllegalArgument,
                        "illegal ROM address");
}

c6502_byte_t MMC1::readRAM(c6502_word_t addr)
{
    if (addr >= 0x6000u && addr <= 0x7FFFu && numRAMs() >= 1)
    {
        return ramBank(0).Read(addr - 0x6000u);
    }
    else
        throw Exception { Exception::IllegalOperation,
                          "this MMC1 controller has no RAM" };
}

c6502_byte_t MMC1::readVROM(c6502_word_t addr)
{
    if (addr >= 0x2000u)
        throw Exception { Exception::IllegalArgument,
                         "illegal VROM address" };

    // For not we only support 8k banks
    assert(m_modeChr == 0u);
    return vromBank(m_curChr[0]).Read(addr);
}

Mirroring MMC1::updateMirroring(Mirroring cur) noexcept
{
    return m_mirrOverride.value(cur);
}

void MMC1::writeRAM(c6502_word_t addr, c6502_byte_t val)
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
            m_shiftReg = 0u;

            // Lock last 16kB ROM bank to 0xC000
            m_modePrg = 3;
        }
        else
        {
            if (m_nWrites++ < 4)
                m_shiftReg = (m_shiftReg << 1u) | (val & 0x01u);
            else
            {
                const auto finVal = (m_shiftReg << 1u) | (val & 0x01u);
                writeRegister(addr & 0xE000u, finVal);
                m_shiftReg = 0u;
                m_nWrites = 0;
            }
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
        m_curChr[0] = val & 0x1Fu;
    else if (addr < 0xE000u)
        // CHR1 bank selector
        m_curChr[1] = val & 0x1Fu;
    else
    {
        // PRG bank
        m_curPrg = val & 0xFu;

        // TODO: MMC1A, MMC1B logic
    }
}
