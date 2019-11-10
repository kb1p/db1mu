#ifndef __MAPPERS_H__
#define __MAPPERS_H__

#include "Cartridge.h"

class DefaultMapper: public Mapper
{
public:
    using Mapper::Mapper;

    c6502_byte_t readROM(c6502_word_t addr) override;

    c6502_byte_t readRAM(c6502_word_t addr) override;

    c6502_byte_t readVROM(c6502_word_t addr) override;

    /* N.B.: some addresses control mapper behaviour (i. e.
     * force bank switching) so, despite the memory itself is r/o,
     * this operation with the mapper is legal.
     */
    virtual void writeRAM(c6502_word_t addr, c6502_byte_t val) override;

    void flash(c6502_word_t addr, c6502_byte_t *p, c6502_d_word_t size);
};

#endif
