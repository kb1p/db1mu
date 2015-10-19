/* 
 * Implements memory (RAM/ROM) objects
 */

#ifndef STORAGE_H
#define	STORAGE_H

#include "common.h"

template <c6502_d_word_t SIZE>
class Storage
{
    public:
    c6502_byte_t Read(c6502_word_t addr);
    void Write(c6502_word_t addr, c6502_byte_t val);
private:
    c6502_byte_t m_mem[SIZE];
};

template <c6502_d_word_t SIZE>
c6502_byte_t Storage<SIZE>::Read(c6502_word_t addr)
{
    return m_mem[addr];
}

template <c6502_d_word_t SIZE>
void Storage<SIZE>::Write(c6502_word_t addr, c6502_byte_t val)
{
    m_mem[addr] = val;
}

#endif	// STORAGE_H

