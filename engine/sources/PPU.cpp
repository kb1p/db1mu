#include "PPU.h"
#include "log.h"

template <c6502_byte_t POS>
constexpr c6502_byte_t bit() noexcept
{
    return 1u << POS;
}

template <c6502_byte_t POS>
constexpr bool test(c6502_byte_t v) noexcept
{
    return (v & (1u << POS)) != 0;
}

c6502_byte_t PPU::readRegister(c6502_word_t n) noexcept
{
    c6502_byte_t rv = 0;
    switch (n)
    {
        case STATE:
            if (!m_st.enableWrite)
                rv |= bit<4>();
            if (m_st.over8sprites)
                rv |= bit<5>();
            if (m_st.sprite0)
            {
                rv |= bit<6>();
                m_st.sprite0 = false;
            }
            if (m_st.vblank)
            {
                rv |= bit<7>();
                m_st.vblank = false;
            }
            break;
        case SPRMEM_DATA:
            rv = m_bus.readSpriteMem(m_st.sprmemAddr++);
            break;
        case VIDMEM_DATA:
            rv = m_bus.readVideoMem(m_st.vramAddr);
            if (!m_st.vramReadError)
                m_st.vramAddr += m_st.addrIncr;
            else
                m_st.vramReadError = false;
            break;
        default:
            assert(false && "Illegal PPU register for reading");
    }

    Log::v("Reading value %X from PPU register #%d", rv, n);
    return rv;
}

void PPU::writeRegister(c6502_word_t n, c6502_byte_t val) noexcept
{
    Log::v("Writing value %X to PPU register #%d", val, n);
    switch (n)
    {
        case CONTROL1:
            switch (val & 0b11u)
            {
                case 0b00u:
                    m_st.activePage = 0x2000u;
                    break;
                case 0b01u:
                    m_st.activePage = 0x2400u;
                    break;
                case 0b10u:
                    m_st.activePage = 0x2800u;
                    break;
                case 0b11u:
                    m_st.activePage = 0x2C00u;
            }
            m_st.addrIncr = test<2>(val) ? 32u : 1u;
            m_st.baSprites = test<3>(val) ? 0x1000u : 0;
            m_st.baBkgnd = test<4>(val) ? 0x1000u : 0;
            m_st.bigSprites = test<5>(val);
            m_st.enableNMI = test<7>(val);

            // DEBUG
            Log::v("active page = %X, "
                   "addr increment = %d, "
                   "sprites chargen addr = %X, "
                   "bg chargen addr = %X, "
                   "big sprites = %d, "
                   "NMI enabled = %d, ",
                   m_st.activePage,
                   m_st.addrIncr,
                   m_st.baSprites,
                   m_st.baBkgnd,
                   m_st.bigSprites,
                   m_st.enableNMI);
            break;
        case CONTROL2:
            m_st.fullBacgroundVisible = test<1>(val);
            m_st.allSpritesVisible = test<2>(val);
            m_st.backgroundVisible = test<3>(val);
            m_st.spritesVisible = test<4>(val);

            // Debug
            Log::v("bg visible = %d, "
                   "full bg = %d, "
                   "sprites visible = %d, "
                   "all sprites = %d, ",
                   m_st.backgroundVisible,
                   m_st.fullBacgroundVisible,
                   m_st.spritesVisible,
                   m_st.allSpritesVisible);
            break;
        case SPRMEM_ADDR:
            m_st.sprmemAddr = val;
            Log::v("sprite address = %X", m_st.sprmemAddr);
            break;
        case SPRMEM_DATA:
            Log::v("write to sprite address = %X", m_st.sprmemAddr);
            m_bus.writeSpriteMem(m_st.sprmemAddr++, val);
            break;
        case VIDMEM_ADDR:
            m_st.vramAddr <<= 8;
            m_st.vramAddr = (m_st.vramAddr & 0xFF00u) | (val & 0xFFu);

            // Read error doesn't happen during palette access
            m_st.vramReadError = m_st.vramAddr < 0x3F00u || m_st.vramAddr >= 0x3F20u;
            Log::v("vram address = %X, read error = %d", m_st.vramAddr, m_st.vramReadError);
            break;
        case VIDMEM_DATA:
            Log::v("write to vram address = %X", m_st.vramAddr);
            m_bus.writeVideoMem(m_st.vramAddr, val);
            m_st.vramAddr += m_st.addrIncr;
            break;
        case SCROLL:
            if (m_currScrollReg ^= 1)
            {
                m_st.scrollH = val;
                Log::v("hscroll = %d", m_st.scrollH);
            }
            else
            {
                m_st.scrollV = val;
                Log::v("vscroll = %d", m_st.scrollV);
            }
            break;
        default:
            assert(false && "Illegal PPU register for writing");
    }
}

void PPU::onBeginVblank() noexcept
{
    m_st.enableWrite = true;
    m_st.vblank = true;
    m_st.sprite0 = false;
}

void PPU::onEndVblank() noexcept
{
    m_st.enableWrite = false;
    m_st.vblank = false;
}

template <typename T, int N>
int indexOf(T ndl, const T (&hstk)[N]) noexcept
{
    int r = -1;
    for (int i = 0; r < 0 && i < N; i++)
        if (hstk[i] == ndl)
            r = i;
    return r;
}

void expandSymbol(c6502_byte_t (&sym)[64],
                  c6502_byte_t clrHi,
                  c6502_word_t palAddr,
                  Bus bus)
    noexcept
{
    // Combine color values
    clrHi <<= 2;
    for (auto &pt: sym)
        if (pt > 0)
            pt = bus.readVideoMem(palAddr + (pt | clrHi)) | 0b11000000u;
}

void PPU::startFrame() noexcept
{
    m_currLine = 0;
    m_pBackend->setBackground(m_bus.readVideoMem(0x3F00u));
}

void PPU::drawNextLine() noexcept
{
    c6502_byte_t sym[64];

    // Render background
    const bool skipTopAndBottom = m_bus.getMode() == OutputMode::NTSC;
    if (m_st.backgroundVisible && m_currLine % 8 == 7 &&
        (!skipTopAndBottom || (m_currLine >= 8 && m_currLine <= 232)))
    {
        static constexpr c6502_word_t PAGE_LAYOUT[] = {
            0x2800u, 0x2C00u,
            0x2000u, 0x2400u
        };

        // Index of the active page in PAGE_LAYOUT
        const int apn = indexOf(m_st.activePage, PAGE_LAYOUT);
        assert(apn >= 0);

        // Render background image
        // Palette: 0x3F00
        const int t = m_st.scrollV,
                  l = m_st.scrollH,
                  vo = t % 8,
                  ho = l % 8;
        constexpr int ppr = 256,
                      ppc = 240;

        const int y = m_currLine - 7,
                  sy = y + t;
        for (int c = 0; c < 32; c++)
        {
            if (!m_st.fullBacgroundVisible && c == 0)
                continue;

            const int x = c * 8,
                      sx = x + l;
            const auto pageAddr = PAGE_LAYOUT[(apn + sy / ppc * 2 + sx / ppr) % 4];

            const auto psx = sx % ppr,                   // page x coordinate
                       psy = sy % ppc,                   // page y coordinate
                       indc = (psy / 8) * 32 + psx / 8,  // index in character area
                       inda = (psy / 32) * 8 + psx / 32; // index in attributes area

            // Read color information from character area
            const auto charNum = m_bus.readVideoMem(pageAddr + indc);
            readCharacter(charNum, sym, m_st.baBkgnd, false, false);

            // Read color information from attribute area
            const auto clrGrp = m_bus.readVideoMem(pageAddr + 960 + inda);
            const auto offInGrp = sy / 16 % 2 * 2 + sx / 16 % 2;
            const c6502_byte_t clrHi = (clrGrp >> (offInGrp << 1)) & 0b11u;

            expandSymbol(sym, clrHi, PAL_BG, m_bus);

            // Load character / attribute data
            m_pBackend->setSymbol(RenderingBackend::Layer::BACKGROUND,
                                    x - ho, y - vo,
                                    sym);
        }
    }

    // Render sprites
    if (m_st.spritesVisible)
    {
        // Sprites on line counter
        int nSprites = 0;
        const c6502_byte_t lastSpriteLine = m_st.bigSprites ? 15u : 7u;
        for (c6502_word_t ns = 0; ns < 64u; ns++)
        {
            const auto i = (63u - ns) * 4u;
            const auto y = m_bus.readSpriteMem(i),
                       nChar = m_bus.readSpriteMem(i + 1),
                       attrs = m_bus.readSpriteMem(i + 2),
                       x = m_bus.readSpriteMem(i + 3);

            if (y + lastSpriteLine != m_currLine ||
                (!m_st.allSpritesVisible && (x >> 3) == 0))
                continue;
            
            const auto lyr = test<5>(attrs) ?
                             RenderingBackend::Layer::BEHIND :
                             RenderingBackend::Layer::FRONT;
            const bool fliph = test<6>(attrs),
                       flipv = test<7>(attrs);
            const c6502_byte_t clrHi = attrs & 0b11u;

            if (!m_st.bigSprites)
            {
                readCharacter(nChar, sym, m_st.baSprites, fliph, flipv);

                expandSymbol(sym, clrHi, PAL_SPR, m_bus);

                // Read symbol, parse attributes
                m_pBackend->setSymbol(lyr, x, y, sym);
            }
            else
            {
                const auto e = nChar % 2;
                const auto baddr = e == 0 ? 0u : 0x1000u;
                readCharacter(nChar - e, sym, baddr, fliph, flipv);
                expandSymbol(sym, clrHi, PAL_SPR, m_bus);
                m_pBackend->setSymbol(lyr, x, y, sym);

                readCharacter(nChar + 1 - e, sym, baddr, fliph, flipv);
                expandSymbol(sym, clrHi, PAL_SPR, m_bus);
                m_pBackend->setSymbol(lyr, x, y + 8, sym);
            }

            nSprites++;
            if (i == 0u)
                m_st.sprite0 = true;
        }
        m_st.over8sprites = nSprites > 8;
    }

    m_currLine++;
}

void PPU::endFrame() noexcept
{
    m_pBackend->draw();
}

void PPU::readCharacter(c6502_word_t ind,
                        c6502_byte_t (&sym)[64],
                        const c6502_word_t baseAddr,
                        const bool fliph,
                        const bool flipv) noexcept
{
    const auto ba = baseAddr + ind * 16;
    for (c6502_word_t i = 0; i < 8; i++)
    {
        const auto r0 = m_bus.readVideoMem(ba + i),
                   r1 = m_bus.readVideoMem(ba + i + 8);
        const auto off = (flipv ? 7 - i : i) * 8;
        for (c6502_word_t j = 0; j < 8; j++)
        {
            auto &d = sym[off + (fliph ? j : 7 - j)];
            d = (((r1 >> j) & 1u) << 1) | ((r0 >> j) & 1u);
        }
    }
}
