#include "PPU.h"
#include "log.h"

template <unsigned POS>
constexpr c6502_byte_t bit() noexcept
{
    return 1u << POS;
}

template <unsigned POS>
constexpr bool test(c6502_byte_t v) noexcept
{
    return (v & (1u << POS)) != 0;
}

c6502_byte_t PPU::readRegister(c6502_word_t n) noexcept
{
    Log::v("Reading PPU register #%d", n);
    
    c6502_byte_t rv = 0;
    switch (n)
    {
        case STATE:
            if (!m_enableWrite)
                rv |= bit<4>();
            if (m_spritesOnLine > 8)
                rv |= bit<5>();
            if (m_sprite0)
                rv |= bit<6>();
            if (!m_busy)
                rv |= bit<7>();
            break;
        case SPRMEM_DATA:
            rv = m_spriteMem.Read(m_sprmemAddr++);
            break;
        case VIDMEM_DATA:
            rv = readVRAM(m_vramAddr);
            if (!m_vramReadError)
                m_vramAddr += m_addrIncr;
            else
                m_vramReadError = false;
            break;
        default:
            assert(false && "Illegal PPU register for reading");
    }

    return rv;
}

void PPU::writeRegister(c6502_word_t n, c6502_byte_t val) noexcept
{
    Log::v("Writing value %d to PPU register #%d", val, n);
    switch (n)
    {
        case CONTROL1:
            switch (val & 0b11u)
            {
                case 0b00u:
                    m_activePage = 0x2000u;
                    break;
                case 0b01u:
                    m_activePage = 0x2400u;
                    break;
                case 0b10u:
                    m_activePage = 0x2800u;
                    break;
                case 0b11u:
                    m_activePage = 0x2C00u;
            }
            m_addrIncr = test<2>(val) ? 32u : 1u;
            m_baSprites = test<3>(val) ? 0x1000u : 0;
            m_baBkgnd = test<4>(val) ? 0x1000u : 0;
            m_bigSprites = test<5>(val);
            m_enableNMI = test<7>(val);
            break;
        case CONTROL2:
            m_fullBacgroundVisible = test<1>(val);
            m_allSpritesVisible = test<2>(val);
            m_backgroundVisible = test<3>(val);
            m_spritesVisible = test<4>(val);
            break;
        case SPRMEM_ADDR:
            m_sprmemAddr = val;
            break;
        case SPRMEM_DATA:
            m_spriteMem.Write(m_sprmemAddr++, val);
            break;
        case VIDMEM_ADDR:
            m_vramAddr <<= 8;
            m_vramAddr = (m_vramAddr & 0xFF00u) | (val & 0xFFu);
            m_vramReadError = true;
            break;
        case VIDMEM_DATA:
            writeVRAM(m_vramAddr, val);
            m_vramAddr += m_addrIncr;
            break;
        case SCROLL:
            if (m_currScrollReg ^= 1)
                m_scrollV = val;
            else
                m_scrollH = val;
            break;
        default:
            assert(false && "Illegal PPU register for writing");
    }
}

void PPU::update() noexcept
{
    m_busy = true;
    buildImage();
    m_busy = false;

    if (m_enableNMI)
        m_bus.generateNMI();
}

void PPU::buildImage() noexcept
{
    static const c6502_word_t LAYOUT[2][2] = {
        { 0x2800u, 0x2C00u },
        { 0x2000u, 0x2400u }
    };

    c6502_byte_t sym[64];
    m_pBackend->setBackground(readVRAM(0x3F00u));

    if (m_backgroundVisible)
    {
        // Render background image
        // Palette: 0x3F00
        const int t = m_scrollV / 30,
                  l = m_scrollH / 32;
        for (int r = 0; r < 30; r++)
        {
            const int y = r * 8 - t;
            for (int c = 0; c < 32; c++)
            {
                const int x = c * 8 - l;
                const auto page = LAYOUT[y / m_ppc][x / m_ppr];
                // Determine page / offset
                // Load character / attribute data
                m_pBackend->setSymbol(RenderingBackend::Layer::BACKGROUND,
                                      x, y,
                                      sym);
            }
        }
    }

    if (m_spritesVisible)
    {
        // Render high priority sprites
        // Palette: 0x3F10
        for (c6502_word_t i = 0; i < 64u; i++)
        {
            const auto y = m_spriteMem.Read(i),
                       nChar = m_spriteMem.Read(i + 1),
                       attrs = m_spriteMem.Read(i + 2),
                       x = m_spriteMem.Read(i + 3);
            const auto lyr = test<5>(attrs) ?
                             RenderingBackend::Layer::FRONT :
                             RenderingBackend::Layer::BEHIND;
            const c6502_byte_t clrIndHi = (attrs & 0b11u) << 2;

            // Read symbol, parse attributes
            m_pBackend->setSymbol(lyr, x, y, sym);
        }
    }

    m_pBackend->draw();
}
