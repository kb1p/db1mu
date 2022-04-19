#include "Cartridge.h"
#include "log.h"

// Mappers
#include "mappers/nrom.h"
#include "mappers/mmc1.h"

#include <algorithm>
#include <memory>

Mapper::Mapper(int nROMs, int nVROMs, int nRAMs):
    m_nROMs(nROMs),
    m_nVROMs(nVROMs),
    m_nRAMs(nRAMs)
{
    m_pROM = new ROM_BANK[nROMs];
    if (nVROMs > 0)
        m_pVROM = new VROM_BANK[nVROMs];
    if (nRAMs > 0)
        m_pRAM = new RAM_BANK[nRAMs];
}

Mapper::~Mapper()
{
    delete[] m_pROM;
    delete[] m_pVROM;
    delete[] m_pRAM;
}

void Mapper::setROMBank(int n, const c6502_byte_t *p)
{
    assert(m_pROM);
    assert(n >= 0 && n < m_nROMs);
    m_pROM[n].Write(0, p, ROM_SIZE);
}

void Mapper::setVROMBank(int n, const c6502_byte_t *p)
{
    assert(m_pVROM);
    assert(n >= 0 && n < m_nVROMs);
    m_pVROM[n].Write(0, p, VROM_SIZE);
}

void Cartrige::setTrainer(const c6502_byte_t tr[512])
{
    if (!m_pTrainer)
        m_pTrainer = new c6502_byte_t[512];
    memcpy(m_pTrainer, tr, 512);
}

void Cartrige::setMapper(uint8_t type,
                         int nROMs,
                         int nVROMs,
                         int nRAMs)
{
    Log::i("[cart] mapper type = %u, # ROMs = %d, # CHRs = %d, # RAMs = %d",
           type, nROMs, nVROMs, nRAMs);

    std::unique_ptr<Mapper> tmp;
    switch (type)
    {
        case Mapper::Default:
            tmp.reset(new DefaultMapper { nROMs, nVROMs });
            break;
        case Mapper::MMC1:
            tmp.reset(new MMC1 { nROMs, nVROMs, nRAMs });
            break;
        default:
            throw Exception(Exception::IllegalArgument,
                            "mapper type is not supported");
    }

    if (tmp)
    {
        delete m_pMapper;
        m_pMapper = tmp.release();
    }
}


