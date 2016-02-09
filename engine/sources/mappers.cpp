#include "mappers.h"

c6502_byte_t DefaultMapper::read(c6502_word_t addr)
{
    if (addr >= 0xC000)
        return m_pROM[1].Read(addr - 0xC000);
    else if (addr >= 0x8000)
        return m_pROM[0].Read(addr - 0x8000);
    else
        throw Exception(Exception::IllegalArgument,
                        "access to RAM (not supported)");
}

void DefaultMapper::write(c6502_word_t addr, c6502_byte_t val)
{
    throw Exception(Exception::IllegalOperation,
                    "writing into read-only memory");
}

void DefaultMapper::flash(c6502_word_t addr, c6502_byte_t* p, c6502_d_word_t size)
{
    if (addr >= 0xC000)
    {
        addr -= 0xC000;
        if (size > Mapper::ROM_SIZE - addr)
            throw Exception(Exception::SizeOverflow,
                            "not enough ROM space");
        m_pROM[1].Write(addr, p, size);
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
        m_pROM[0].Write(addr, p, space);
    }
    else
        throw Exception(Exception::IllegalArgument, "address outside the ROM space");
}

