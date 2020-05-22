#include "PPU.h"
#include "bus.h"
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
                rv |= bit<6>();
            if (m_st.vblank)
                rv |= bit<7>();

            m_st.vblank = false;
            m_scrollSwitch = 0;
            break;
        case SPRMEM_DATA:
            rv = bus().readSpriteMem(m_st.sprmemAddr++);
            break;
        case VIDMEM_DATA:
            {
                const auto v = bus().readVideoMem(m_st.vramAddr);
                if (m_st.vramAddr < 0x3F00u)
                {
                    rv = m_st.vramReadBuf;
                    m_st.vramReadBuf = v;
                }
                else
                    rv = v;
                m_st.vramAddr += m_st.addrIncr;
            }
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
            m_st.activePageIndex = val & 0b11u;
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
                   m_st.activePage(),
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
            bus().writeSpriteMem(m_st.sprmemAddr++, val);
            break;
        case VIDMEM_ADDR:
            m_st.vramAddr <<= 8;
            m_st.vramAddr &= 0xFF00u;
            m_st.vramAddr |= val & 0xFFu;

            Log::v("vram address = %X", m_st.vramAddr);
            break;
        case VIDMEM_DATA:
            if (m_st.enableWrite)
            {
                Log::v("write to vram address = %X", m_st.vramAddr);
                bus().writeVideoMem(m_st.vramAddr, val);
                m_st.vramAddr += m_st.addrIncr;
            }
            break;
        case SCROLL:
            if (m_scrollSwitch == 0)
            {
                m_st.scrollH = val;
                Log::v("hscroll = %d", m_st.scrollH);
            }
            else
            {
                m_st.scrollV = val;
                Log::v("vscroll = %d", m_st.scrollV);
            }
            m_scrollSwitch ^= 1;
            break;
        default:
            assert(false && "Illegal PPU register for writing");
    }
}

void PPU::onBeginVblank() noexcept
{
    m_st.enableWrite = true;
    m_st.vblank = true;
}

void PPU::onEndVblank() noexcept
{
    m_st.enableWrite = false;
    m_st.vblank = false;
}

PPU::PageTileInfo PPU::getTile(const int sx, const int sy) noexcept
{
    static constexpr c6502_word_t PAGE_LAYOUT[2][2] = {
        { 0x2000u, 0x2400u },
        { 0x2800u, 0x2C00u }
    };

    PageTileInfo ti;
    ti.pageAddr = PAGE_LAYOUT[sy / PPC % 2][sx / PPR % 2];

    const auto psx = sx % PPR,  // page x coordinate
               psy = sy % PPC;  // page y coordinate

    ti.charIndex = (psy / 8) * 32 + psx / 8,  // index in character area
    ti.attrIndex = (psy / 32) * 8 + psx / 32; // index in attributes area

    return ti;
}

void PPU::startFrame() noexcept
{
    m_currLine = 0;
    m_frameVScroll = m_st.scrollV;
    m_st.sprite0 = false;
    m_st.over8sprites = false;
}

void PPU::drawNextLine() noexcept
{
    const bool NTSCLineSkip = bus().getMode() == OutputMode::NTSC &&
                              (m_currLine < 8 || m_currLine > 231);

    const int t = m_frameVScroll + ((m_st.activePageIndex >> 1u) & 1u) * PPC,
              l = m_st.scrollH + (m_st.activePageIndex & 1u) * PPR,
              vo = t % 8,
              ho = l % 8;

    // Visible line + 1 tile gap for BG scrolling + 1 tile gap for sprite clipping to the right
    static constexpr auto LINE_WIDTH = PPR + 8 + 8;
    c6502_byte_t lnData[LINE_WIDTH];

    // Fill with background color
    memset(lnData, TRANSPARENT, LINE_WIDTH);

    // If PPU is turned off, writing to VRAM is possible
    m_st.enableWrite = NTSCLineSkip || (!m_st.backgroundVisible && !m_st.spritesVisible);

    // Render background
    if (!NTSCLineSkip && m_st.backgroundVisible)
    {
        const int y = m_currLine,
                  sy = y + t;
        for (int c = 0; c < 34; c++)
        {
            if (!m_st.fullBacgroundVisible && c == 0)
                continue;

            const int x = c * 8,
                      sx = x + l;

            const auto ti = getTile(sx, sy);

            // Read character index from character area
            const auto charNum = bus().readVideoMem(ti.characterAddress());

            // Read color information from attribute area
            const auto clrGrp = bus().readVideoMem(ti.attributeAddress());
            const auto offInGrp = sy / 16 % 2 * 2 + sx / 16 % 2;
            const c6502_byte_t clrHi = (clrGrp >> (offInGrp << 1u)) & 0b11u;

            // Load character / attribute data
            assert(x <= 256 + 8);
            readCharacterLine(lnData + x, charNum, (y + vo) % 8, m_st.baBkgnd, false, false);
            expandColor(lnData + x, clrHi, PAL_BG);
        }
    }

    // Render sprites
    if (!NTSCLineSkip && m_st.spritesVisible)
    {
        c6502_byte_t sprLnData[8];

        // Sprites on line counter
        int nSprites = 0;
        const c6502_byte_t lastSpriteLine = m_st.bigSprites ? 15u : 7u;
        for (int ns = 63; ns >= 0; ns--)
        {
            const auto sa = static_cast<c6502_word_t>(ns * 4u);
            const auto y = static_cast<c6502_byte_t>(bus().readSpriteMem(sa) + 1u),
                       nChar = bus().readSpriteMem(sa + 1),
                       attrs = bus().readSpriteMem(sa + 2),
                       x = bus().readSpriteMem(sa + 3);

            if (m_currLine < y || m_currLine > y + lastSpriteLine ||
                (!m_st.allSpritesVisible && (x >> 3) == 0))
                continue;

            const bool behindBg = test<5>(attrs);
            const bool fliph = test<6>(attrs),
                       flipv = test<7>(attrs);
            const c6502_byte_t clrHi = attrs & 0b11u;

            auto nEffChar = nChar;
            auto nEffCharLn = m_currLine - y;
            auto baddr = m_st.baSprites;
            if (m_st.bigSprites)
            {
                const auto e = nChar % 2;
                baddr = e == 0 ? 0u : 0x1000u;
                nEffChar = nChar - e;
                if (nEffCharLn >= 8)
                {
                    nEffChar++;
                    nEffCharLn -= 8;
                }
            }

            // Read symbol, parse attributes
            readCharacterLine(sprLnData, nEffChar, nEffCharLn, baddr, fliph, flipv);
            expandColor(sprLnData, clrHi, PAL_SPR);

            // Compose sprite and background data
            for (int i = 0; i < 8; i++)
            {
                assert(x + ho <= 256 + 8);
                auto &bp = lnData[x + ho + i],
                     &sp = sprLnData[i];
                if (sp != TRANSPARENT)
                {
                    if (!behindBg || bp == TRANSPARENT)
                        bp = sp;

                    // Sprite 0 hit test
                    if (ns == 0 && bp != TRANSPARENT && x < 255u)
                        m_st.sprite0 = true;
                }
            }

            nSprites++;
        }
        if (nSprites > 8)
            m_st.over8sprites = true;
    }

    m_pBackend->setLine(m_currLine, lnData + ho, bus().readVideoMem(0x3F00u));

    m_currLine++;
}

void PPU::endFrame() noexcept
{
    m_pBackend->draw();
}

void PPU::readCharacterLine(c6502_byte_t *line,
                            const c6502_word_t charInd,
                            const c6502_word_t lineInd,
                            const c6502_word_t baseAddr,
                            const bool fliph,
                            const bool flipv)
    noexcept
{
    assert(line != nullptr);
    assert(lineInd < 8u);

    const auto ba = baseAddr + charInd * 16u + (flipv ? 7u - lineInd : lineInd);
    const auto r0 = bus().readVideoMem(ba),
               r1 = bus().readVideoMem(ba + 8u);
    for (c6502_word_t j = 0; j < 8; j++)
    {
        const auto fj = (fliph ? j : 7u - j);
        *line++ = (((r1 >> fj) & 1u) << 1u) | ((r0 >> fj) & 1u);
    }
}

void PPU::expandColor(c6502_byte_t *p,
                      c6502_byte_t clrHi,
                      const c6502_word_t palAddr)
    noexcept
{
    assert(p != nullptr);

    // Combine color values
    clrHi <<= 2;
    for (int i = 0; i < 8; i++, p++)
        *p = *p > 0 ? bus().readVideoMem(palAddr + (*p | clrHi)) : TRANSPARENT;
}
