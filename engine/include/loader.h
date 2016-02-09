#ifndef __LOADER_H__
#define __LOADER_H__

#include "Cartridge.h"
#include <istream>

/*!
 * NES file format
 * ===
 *
 * See http://fms.komkon.org/EMUL8/NES.html#LABM
 *
 * Byte     Contents
 * ---------------------------------------------------------------------------
 * 0-3      String "NES^Z" used to recognize .NES files.
 * 4        Number of 16kB ROM banks.
 * 5        Number of 8kB VROM banks.
 * 6        bit 0     1 for vertical mirroring, 0 for horizontal mirroring.
 *          bit 1     1 for battery-backed RAM at $6000-$7FFF.
 *          bit 2     1 for a 512-byte trainer at $7000-$71FF.
 *          bit 3     1 for a four-screen VRAM layout. 
 *          bit 4-7   Four lower bits of ROM Mapper Type.
 * 7        bit 0     1 for VS-System cartridges.
 *          bit 1-3   Reserved, must be zeroes!
 *          bit 4-7   Four higher bits of ROM Mapper Type.
 * 8        Number of 8kB RAM banks. For compatibility with the previous
 *          versions of the .NES format, assume 1x8kB RAM page when this
 *          byte is zero.
 * 9        bit 0     1 for PAL cartridges, otherwise assume NTSC.
 *          bit 1-7   Reserved, must be zeroes!
 * 10-15    Reserved, must be zeroes!
 * 16-...   ROM banks, in ascending order. If a trainer is present, its
 *          512 bytes precede the ROM bank contents.
 * ...-EOF  VROM banks, in ascending order.
 * ---------------------------------------------------------------------------
 */
struct NESHeader
{
    char magic[4];
    uint8_t nROMs;
    uint8_t nVROMs;
    union
    {
        uint8_t c1;
        struct
        {
            uint8_t mirror: 1,
                    hasRAM: 1,
                    hasTrainer: 1,
                    fourScreenVRAM: 1,
                    mapperLo: 4;
        };
    };
    union
    {
        uint8_t c2;
        struct
        {
            uint8_t vssystem: 1,
                    zeroes: 3,          // should be checked
                    mapperHi: 4;
        };
    };
    uint8_t nRAMs;
    uint8_t isPal;      // only {0, 1}, otherwise error

    bool checkValid() const;
};

static_assert(sizeof(NESHeader) == 10, "NESHeader is padded incorrectly");

/*!
 * \brief Allows reading of NES ROMs and raw data blocks from a file to
 * the cartridge.
 */
class ROMLoader
{
    Cartrige &m_cart;
    NESHeader m_hdr;

public:
    ROMLoader(const ROMLoader&) = delete;
    ROMLoader(ROMLoader&&) = delete;
    ROMLoader &operator=(const ROMLoader&) = delete;
    ROMLoader &operator=(ROMLoader&&) = delete;

    ROMLoader(Cartrige &cart);

    /*!
     * Loads the NES ROM.
     * \param file NES file path.
     * \see http://fms.komkon.org/EMUL8/NES.html#LABM
     */
    void loadNES(const char *file) throw(Exception);

    /*!
     * Load a binary file contents as cartridge ROM data.
     * \param file Raw data file path.
     * \param addr Address (>= 0x8000) of the destination in 6502 address space.
     * \param off Offset from the beginning of the file.
     * \param len Length of block to read.
     */
    void loadRawData(std::istream &in,
                     c6502_word_t addr = 0x8000u,
                     c6502_word_t len = Mapper::ROM_SIZE * 2) throw(Exception);

    const NESHeader &header() const
    {
        return m_hdr;
    }
};

#endif // __LOADER_H__