#include "loader.h"
#include "mappers/nrom.h"
#include <cstring>
#include <fstream>
#include <cassert>
#include <algorithm>

using std::ios;
using std::istream;
using std::ifstream;

bool NESHeader::checkValid() const
{
    static const char MAGIC_VAL[4] = { 'N', 'E', 'S', 0x1A };
    if (memcmp(magic, MAGIC_VAL, 4) != 0)
        return false;

    if (zeroes != 0)
        return false;

    if (nROMs == 0)
        return false;

    return isPal <= 1;
}

ROMLoader::ROMLoader(Cartrige& cart):
    m_cart(cart)
{
}

// Read & validate
static void sread(void *pDest, size_t nBytes, istream &in)
{
    in.read(reinterpret_cast<char*>(pDest), nBytes);
    if (static_cast<size_t>(in.gcount()) < nBytes)
        throw Exception(Exception::IllegalFormat, "unexpected end of file");
}

void ROMLoader::loadRawData(istream &in,
                            c6502_word_t addr,
                            c6502_word_t len)
{
    assert(addr >= 0x8000);

    in.seekg(0, ios::end);
    const c6502_d_word_t sourceSize = std::min(static_cast<c6502_word_t>(in.tellg()), len);
    if (sourceSize > 0x10000u - addr)
        throw Exception(Exception::SizeOverflow, "source data size exceeds ROM banks capacity");

    // Go to the start pf the block
    in.seekg(0, ios::beg);

    m_cart.setMapper(Mapper::Default);
    DefaultMapper *dm = static_cast<DefaultMapper*>(m_cart.mapper());

    c6502_byte_t buf[1024];
    c6502_d_word_t left = sourceSize;
    while (in.good() && left > 0)
    {
        in.read(reinterpret_cast<char*>(buf), 1024);
        c6502_d_word_t read = in.gcount();
        if (read > left)
            // Write the remainder and quit
            read = left;

        dm->flash(addr, buf, read);

        addr += read;
        left -= read;
    }
}

void ROMLoader::loadNES(const char *file)
{
    ifstream in(file, ios::in | ios::binary);
    if (!in.is_open())
        throw Exception(Exception::IOFailure, "unable to open the file");

    // Read & check header
    try
    {
        sread(&m_hdr, sizeof(NESHeader), in);

        if (!m_hdr.checkValid())
            throw Exception(Exception::IllegalFormat, "incorrect NES ROM header");

        c6502_byte_t zeroes[6];
        sread(zeroes, 6, in);

        for (int i = 0; i < 6; i++)
            if (zeroes[i] != 0)
                throw Exception(Exception::IllegalFormat, "unexpected data");

        if (m_hdr.hasTrainer)
        {
            c6502_byte_t train[512];
            sread(train, 512, in);
            m_cart.setTrainer(train);
        }

        if (m_hdr.fourScreenVRAM)
            m_cart.setMirroring(Mirroring::FourScreen);
        else if (m_hdr.mirror)
            m_cart.setMirroring(Mirroring::Vertical);
        else
            m_cart.setMirroring(Mirroring::Horizontal);

        // RAM counter fixup for compatibility
        const int nRAMs = (m_hdr.hasRAM && m_hdr.nRAMs == 0) ? 1 : m_hdr.nRAMs;
        m_cart.setMapper(m_hdr.mapperLo | (m_hdr.mapperHi << 4),
                         m_hdr.nROMs,
                         m_hdr.nVROMs,
                         nRAMs);
        Mapper *map = m_cart.mapper();

        c6502_byte_t rom[Mapper::ROM_SIZE];
        for (int i = 0; i < m_hdr.nROMs; i++)
        {
            sread(rom, Mapper::ROM_SIZE, in);
            map->setROMBank(i, rom);
        }

        c6502_byte_t vrom[Mapper::VROM_SIZE];
        for (int i = 0; i < m_hdr.nVROMs; i++)
        {
            sread(vrom, Mapper::VROM_SIZE, in);
            map->setVROMBank(i, vrom);
        }

        // No data left
        if (in.get() != ifstream::traits_type::eof())
            throw Exception(Exception::IllegalFormat, "enormous file size excession");
    }
    catch (const Exception&)
    {
        in.close();
        throw;
    }
    in.close();
}
