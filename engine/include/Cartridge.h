#ifndef CARTRIDGE_H
#define	CARTRIDGE_H

#include "storage.h"

struct Cartrige
{
    Storage<8 * 1024> wram; // 0x6000 ~ 0x7FFF
    Storage<16 * 1024> rom[2]; // 0x8000 ~ 0xBFFF  &  0xC000 ~ 0xFFFF
};


#endif	// CARTRIDGE_H

