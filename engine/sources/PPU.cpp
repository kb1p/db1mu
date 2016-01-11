#include "PPU.h"

PPU::PPU() { }

PPU::Registers& PPU::GetRegisters()
{
    return m_registers;
}