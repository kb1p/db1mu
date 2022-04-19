#include "mappers/nrom.h"

c6502_byte_t DefaultMapper::readMem(c6502_word_t addr)
{
    if (addr >= 0xC000)
        // Fixed bank
        return romBank(numROMs() - 1).Read(addr - 0xC000);
    else if (addr >= 0x8000)
        // Switchable bank (only one for default mapper)
        return romBank(0).Read(addr - 0x8000);
    else
        throw Exception(Exception::IllegalArgument,
                        "illegal memory address");
}

c6502_byte_t DefaultMapper::readVideoMem(c6502_word_t addr)
{
    assert(numVROMs() == 1);
    assert(addr < 0x2000u);

    // Only one VROM bank for default mapper
    return vromBank(0).Read(addr);
}

void DefaultMapper::writeMem(c6502_word_t, c6502_byte_t)
{
    throw Exception(Exception::IllegalOperation,
                    "default mapper has no RAM");
}

void DefaultMapper::flash(c6502_word_t addr, c6502_byte_t* p, c6502_d_word_t size)
{
    if (addr >= 0xC000)
    {
        addr -= 0xC000;
        if (size > Mapper::ROM_SIZE - addr)
            throw Exception(Exception::SizeOverflow,
                            "not enough ROM space");
        romBank(1).Write(addr, p, size);
    }
    else if (addr >= 0x8000)
    {
        addr -= 0x8000;
        const c6502_d_word_t space =Mapper::ROM_SIZE - addr;
        if (size > space)
        {
            flash(0xC000, p + space, size - space);
            size = space;
        }
        romBank(0).Write(addr, p, space);
    }
    else
        throw Exception(Exception::IllegalArgument, "address outside the ROM space");
}
