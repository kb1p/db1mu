#ifndef CARTRIDGE_H
#define	CARTRIDGE_H

#include "storage.h"

class Mapper
{
public:
    /// Known mappers
    // From http://fms.komkon.org/EMUL8/NES.html#LABM :
    // Mapper#  Name                  Examples
    // ---------------------------------------------------------------------------
    // 0        No mapper             All 32kB ROM + 8kB VROM games
    // 1        Nintendo MMC1         Megaman2, Bomberman2, etc.
    // 2        CNROM switch          Castlevania, LifeForce, etc.
    // 3        UNROM switch          QBert, PipeDream, Cybernoid, many Japanese games
    // 4        Nintendo MMC3         SilverSurfer, SuperContra, Immortal, etc.
    // 5        Nintendo MMC5         Castlevania3
    // 6        FFE F4xxx             F4xxx games off FFE CDROM
    // 7        AOROM switch          WizardsAndWarriors, Solstice, etc.
    // 8        FFE F3xxx             F3xxx games off FFE CDROM
    // 9        Nintendo MMC2         Punchout
    // 10       Nintendo MMC4         Punchout2
    // 11       ColorDreams chip      CrystalMines, TaginDragon, etc.
    // 12     - FFE F6xxx             F6xxx games off FFE CDROM
    // 13       CPROM switch
    // 15       100-in-1 switch       100-in-1 cartridge
    // 16       Bandai chip           Japanese DragonBallZ series, etc.
    // 17       FFE F8xxx             F8xxx games off FFE CDROM
    // 18       Jaleco SS8806 chip    Japanese Baseball3, etc.
    // 19       Namcot 106 chip       Japanese GhostHouse2, Baseball90, etc.
    // 20       Nintendo DiskSystem   Reserved. Don't use this mapper!
    // 21       Konami VRC4a          Japanese WaiWaiWorld2, etc.
    // 22       Konami VRC2a          Japanese TwinBee3
    // 23       Konami VRC2a          Japanese WaiWaiWorld, MoonWindLegend, etc.
    // 24     - Konami VRC6
    // 25       Konami VRC4b
    // 32       Irem G-101 chip       Japanese ImageFight, etc.
    // 33       Taito TC0190/TC0350   Japanese PowerBlazer
    // 34       Nina-1 board          ImpossibleMission2 and DeadlyTowers
    // 64       Tengen RAMBO-1 chip
    // 65       Irem H-3001 chip
    // 66       GNROM switch
    // 67       SunSoft3 chip
    // 68       SunSoft4 chip
    // 69       SunSoft5 FME-7 chip
    // 71       Camerica chip
    // 78       Irem 74HC161/32-based
    // 79       AVE Nina-3 board      KrazyKreatures, DoubleStrike, etc.
    // 81       AVE Nina-6 board      Deathbots, MermaidsOfAtlantis, etc.
    // 91       Pirate HK-SF3 chip
    // ---------------------------------------------------------------------------
    enum ChipType: uint8_t
    {
        Default = 0,
        MMC1 = 1,
        CNROM = 2,
        UNROM = 3,
        MMC3 = 4,
        MMC5 = 5,
        FFE_F4xxx = 6,
        AOROM = 7,
        FFE_F3xxx = 8,
        MMC2 = 9,
        MMC4 = 10,
        ColorDreams = 11,
        FFE_F6xxx = 12,
        CPROM = 13,
        Chip_100in1 = 15,
        Bandai = 16,
        FFE_F8xxx = 17,
        SS8806 = 18,
        Namcot_106 = 19,
        NDS = 20,
        VRC4a = 21,
        VRC2a_1 = 22,
        VRC2a_2 = 23,
        VRC6 = 24,
        VRC4b = 25,
        G101 = 32,
        TC0190_0350 = 33,
        Nina1 = 34,
        RAMBO1 = 64,
        H3001 = 65,
        GNROM = 66,
        SunSoft3 = 67,
        SunSoft4 = 68,
        SunSoft5 = 69,
        Camerica = 71,
        Chip_74HC161 = 78,
        Nina3 = 79,
        Nina6 = 81,
        HK_SF3 = 91
    };

    // Bank sizes
    static constexpr c6502_d_word_t ROM_SIZE = 16 * 1024,
                                    VROM_SIZE = 8 * 1024,
                                    RAM_SIZE = 8 * 1024;

    typedef Storage<ROM_SIZE> ROM_BANK;
    typedef Storage<VROM_SIZE> VROM_BANK;
    typedef Storage<RAM_SIZE> RAM_BANK;

    Mapper(int nROMs, int nVROMs, int nRAMs);
    virtual ~Mapper();

    void addROMBank(const c6502_byte_t *p);
    void addVROMBank(const c6502_byte_t *p);

    virtual c6502_byte_t read(c6502_word_t addr) = 0;

    /* N.B.: some addresses control mapper behaviour (i. e.
     * force bank switching) so, despite the memory itself is r/o,
     * this operation with the mapper is legal.
     */
    virtual void write(c6502_word_t addr, c6502_byte_t val) = 0;

    /* TODO: PPU has an addressing space separate from that of CPU,
     * add routines for PPU reading / writing, e. g. readPPU(),
     * writePPU().
     */

protected:
    const int m_nROMs, m_nVROMs, m_nRAMs;
    int m_curROM = 0, m_curVROM = 0;
    ROM_BANK *m_pROM = nullptr;
    VROM_BANK *m_pVROM = nullptr;
    RAM_BANK *m_pRAM = nullptr;

    friend class Cartrige;
};

enum class Mirroring
{
    Horizontal,
    Vertical,
    FourScreen
};

class Cartrige
{
    Mapper *m_pMapper = nullptr;
    c6502_byte_t *m_pTrainer = nullptr;
    Mirroring m_mirr = Mirroring::Horizontal;

public:
    ~Cartrige()
    {
        delete m_pMapper;
        delete[] m_pTrainer;
    }

    bool isReady() const
    {
        return m_pMapper != nullptr;
    }

    Mapper *mapper() const
    {
        return m_pMapper;
    }

    void setMapper(uint8_t type,
                   int nROMs = 2,
                   int nVROMs = 1,
                   int nRAMs = 0);

    const c6502_byte_t *trainer() const
    {
        return m_pTrainer;
    }

    void setTrainer(const c6502_byte_t tr[512]);

    Mirroring mirroring() const
    {
        return m_mirr;
    }

    void setMirroring(Mirroring m)
    {
        m_mirr = m;
    }

    int numRAMs() const
    {
        assert(m_pMapper);
        return m_pMapper->m_nRAMs;
    }

    int numROMs() const
    {
        assert(m_pMapper);
        return m_pMapper->m_nROMs;
    }

    int numVROMs() const
    {
        assert(m_pMapper);
        return m_pMapper->m_nVROMs;
    }

    c6502_byte_t read(c6502_word_t addr) const
    {
        assert(m_pMapper);
        assert(addr >= 0x6000);
        return m_pMapper->read(addr);
    }

    void write(c6502_word_t addr, c6502_byte_t val)
    {
        assert(m_pMapper);
        assert(addr >= 0x6000);
        m_pMapper->write(addr, val);
    }
};

#endif // CARTRIDGE_H

