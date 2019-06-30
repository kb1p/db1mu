#ifndef __MAPPERS_H__
#define __MAPPERS_H__

#include "Cartridge.h"

class DefaultMapper: public Mapper
{
public:
    using Mapper::Mapper;

    c6502_byte_t read(c6502_word_t addr) override;

    void write(c6502_word_t addr, c6502_byte_t val) override;

    void flash(c6502_word_t addr, c6502_byte_t *p, c6502_d_word_t size);
};

#endif
