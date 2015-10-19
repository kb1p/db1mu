#ifndef PPU_H
#define	PPU_H

#include "storage.h"

class PPU
{
public:
    PPU();

    typedef Storage<0x8> Registers;
    Registers& GetRegisters();
private:
    typedef Storage<0x4000> VRAM;
    VRAM m_vram;
    Registers m_registers;
};

#endif	/* PPU_H */

