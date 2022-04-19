#ifndef __NROM_H__
#define __NROM_H__

#include "Cartridge.h"

class DefaultMapper: public Mapper
{
public:
    DefaultMapper(int nROMs, int nVROMs):
        Mapper { nROMs, nVROMs, 0 }
    {
    }

    c6502_byte_t readMem(c6502_word_t addr) override;

    c6502_byte_t readVideoMem(c6502_word_t addr) override;

    /* N.B.: some addresses control mapper behaviour (i. e.
     * force bank switching) so, despite the memory itself is r/o,
     * this operation with the mapper is legal.
     */
    void writeMem(c6502_word_t addr, c6502_byte_t val) override;

    void flash(c6502_word_t addr, c6502_byte_t *p, c6502_d_word_t size);
};

#endif
