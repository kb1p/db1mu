/* 
 * Implements memory (RAM/ROM) objects
 */

#ifndef STORAGE_H
#define STORAGE_H

#include "common.h"
#include <cstring>
#include <cassert>

template <c6502_d_word_t SIZE>
class Storage
{
public:
    c6502_byte_t Read(c6502_word_t addr) const noexcept;

    void Write(c6502_word_t addr, c6502_byte_t val) noexcept;
    void Write(c6502_word_t addr, const c6502_byte_t *beg, c6502_d_word_t count) noexcept;

    void Clear() noexcept
    {
        memset(m_mem, 0, SIZE);
    }

    template <typename OutStreamT>
    void Save(OutStreamT &strm)
    {
        using CharT = typename OutStreamT::char_type;
        static_assert(SIZE % sizeof(CharT) == 0);
        strm.write(reinterpret_cast<const CharT*>(m_mem),
                   SIZE / sizeof(CharT));
    }

    template <typename InStreamT>
    void Load(InStreamT &strm)
    {
        using CharT = typename InStreamT::char_type;
        static_assert(SIZE % sizeof(CharT) == 0);
        strm.read(reinterpret_cast<CharT*>(m_mem),
                  SIZE / sizeof(CharT));
    }

private:
    c6502_byte_t m_mem[SIZE];
};

template <c6502_d_word_t SIZE>
c6502_byte_t Storage<SIZE>::Read(c6502_word_t addr) const noexcept
{
    assert(addr < SIZE);
    return m_mem[addr];
}

template <c6502_d_word_t SIZE>
void Storage<SIZE>::Write(c6502_word_t addr, c6502_byte_t val) noexcept
{
    assert(addr < SIZE);
    m_mem[addr] = val;
}

template <c6502_d_word_t SIZE>
void Storage<SIZE>::Write(c6502_word_t addr, const c6502_byte_t* beg, c6502_d_word_t count) noexcept
{
    assert(count <= SIZE);
    assert(addr < SIZE);
    memcpy(m_mem + addr, beg, count);
}

#endif	// STORAGE_H

