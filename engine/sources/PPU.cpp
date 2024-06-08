#include "PPU.h"
#include "bus.h"
#include "log.h"

constexpr int RenderingBackend::TEX_WIDTH,
              RenderingBackend::TEX_HEIGHT;

const uint32_t RenderingBackend::s_palette[64] = {
    0b011100111001110u,
    0b001000001110001u,
    0b000000000010101u,
    0b010000000010011u,
    0b100010000001110u,
    0b101010000000010u,
    0b101000000000000u,
    0b011110000100000u,
    0b010000010100000u,
    0b000000100000000u,
    0b000000101000000u,
    0b000000011100010u,
    0b000110011101011u,
    0b000000000000000u,
    0b000000000000000u,
    0b000000000000000u,
    0b101111011110111u,
    0b000000111011101u,
    0b001000011111101u,
    0b100000000011110u,
    0b101110000010111u,
    0b111000000001011u,
    0b110110010100000u,
    0b110010100100001u,
    0b100010111000000u,
    0b000001001000000u,
    0b000001010100000u,
    0b000001001000111u,
    0b000001000010001u,
    0b000000000000000u,
    0b000000000000000u,
    0b000000000000000u,
    0b111111111111111u,
    0b001111011111111u,
    0b010111001011111u,
    0b110011000111111u,
    0b111100111111111u,
    0b111110111010110u,
    0b111110111001100u,
    0b111111001100111u,
    0b111101011100111u,
    0b100001101000010u,
    0b010011101101001u,
    0b010111111110011u,
    0b000001110111011u,
    0b011110111101111u,
    0b000000000000000u,
    0b000000000000000u,
    0b111111111111111u,
    0b101011110011111u,
    0b110001101011111u,
    0b110101100111111u,
    0b111111100011111u,
    0b111111100011011u,
    0b111111011110110u,
    0b111111101110101u,
    0b111111110010100u,
    0b111001111110100u,
    0b101011111010111u,
    0b101101111111001u,
    0b100111111111110u,
    0b110001100011000u,
    0b000000000000000u,
    0b000000000000000u
};

void RenderingBackend::setLineToBuf(uint8_t *dst,
                                    const int n,
                                    const c6502_byte_t *pColorData,
                                    const c6502_byte_t bgColor)
{
    assert(pColorData != nullptr);

    // Convert NES character (NES palette) into tile (RGB palette)
    auto *pDest = dst + (TEX_HEIGHT - 1 - n) * TEX_WIDTH * 4;
    for (int i = 0; i < TEX_WIDTH; i++, pDest += 4)
    {
        const auto c = pColorData[i] != PPU::TRANSPARENT_PXL ? (pColorData[i] & 0x3Fu) : bgColor;
        assert(c < 64);
        const auto s = s_palette[c];
        constexpr unsigned b5m = 0b11111u;
        pDest[0] = static_cast<uint8_t>(divrnd(((s >> 10) & b5m) * 255, 31));
        pDest[1] = static_cast<uint8_t>(divrnd(((s >> 5) & b5m) * 255, 31));
        pDest[2] = static_cast<uint8_t>(divrnd((s & b5m) * 255, 31));
        pDest[3] = 255u;
    }
}

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

// Perform VRAM address coarse X increment with wrapping
c6502_word_t incrWrpAddrHorz(c6502_word_t a) noexcept
{
    constexpr c6502_word_t M_COARSE_X = 0b11111u;

    if ((a & M_COARSE_X) < M_COARSE_X)
        a++;
    else
    {
        a &= ~M_COARSE_X;
        a ^= 0x400u;
    }

    return a;
}

// Perform VRAM address Y increment with wrapping
c6502_word_t incrWrpAddrVert(c6502_word_t a) noexcept
{
    constexpr c6502_word_t M_COARSE_Y = 0b1111100000u,
                           M_FINE_Y = 0b111000000000000u;

    // Increment Y position in memory access register
    if ((a & M_FINE_Y) < M_FINE_Y)
        a += 0b1000000000000u;
    else
    {
        a &= ~M_FINE_Y;
        c6502_word_t crY = (a >> 5u) & 0b11111u;
        switch (crY)
        {
            case 29:
                a ^= 0x800u;
            case 31:
                crY = 0u;
                break;
            default:
                crY++;
        }
        a &= ~M_COARSE_Y;
        a |= (crY << 5u) & M_COARSE_Y;
    }

    return a;
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
            m_st.w = 0;
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
    const bool wasNMIEnabled = m_st.enableNMI;
    switch (n)
    {
        case CONTROL1:
            m_st.tmpAddr &= 0b1111001111111111u;
            m_st.tmpAddr |= ((val & 0b11u) << 10u);
            m_st.addrIncr = test<2>(val) ? 32u : 1u;
            m_st.baSprites = test<3>(val) ? 0x1000u : 0;
            m_st.baBkgnd = test<4>(val) ? 0x1000u : 0;
            m_st.bigSprites = test<5>(val);
            m_st.enableNMI = test<7>(val);

            if (m_st.vblank && !wasNMIEnabled && m_st.enableNMI)
                bus().triggerNMI();

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
            if (m_st.w == 0)
            {
                m_st.tmpAddr &= 0x00FFu;
                m_st.tmpAddr |= (val & 0x3Fu) << 8u;
            }
            else
            {
                m_st.tmpAddr &= 0xFF00u;
                m_st.tmpAddr |= val;
                m_st.vramAddr = m_st.tmpAddr;
            }
            m_st.w ^= 1;

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
            if (m_st.w == 0)
            {
                m_st.fineX = val & 0b111u;
                m_st.tmpAddr &= 0b1111111111100000u;
                m_st.tmpAddr |= (val >> 3u) & 0b11111u;
                Log::v("hscroll = %d", val);
            }
            else
            {
                m_st.tmpAddr &= 0b1000110000011111u;
                m_st.tmpAddr |= (val & 0b111u) << 12u;
                m_st.tmpAddr |= (val & 0b11111000u) << 2u;
                Log::v("vscroll = %d", val);
            }
            m_st.w ^= 1;
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

void PPU::startFrame() noexcept
{
    m_currLine = 0;
    m_st.sprite0 = false;
    m_st.over8sprites = false;

    // Pre-rendering scanline emulation
    if (m_st.backgroundVisible || m_st.spritesVisible)
        m_st.vramAddr = m_st.tmpAddr;
}

void PPU::drawNextLine() noexcept
{
    const bool NTSCLineSkip = bus().getMode() == OutputMode::NTSC &&
                              (m_currLine < 8 || m_currLine > 231);

    const c6502_word_t fineY = (m_st.vramAddr >> 12u) & 0b111u,
                       fineX = m_st.fineX;

    // Visible line + 1 tile gap for BG scrolling + 1 tile gap for sprite clipping to the right
    static constexpr auto LINE_WIDTH = PPR + 8 + 8;
    c6502_byte_t lnData[LINE_WIDTH];

    // Fill with background color
    memset(lnData, TRANSPARENT_PXL, LINE_WIDTH);

    // If PPU is turned off, writing to VRAM is possible
    const bool enableRendering = m_st.backgroundVisible || m_st.spritesVisible;
    m_st.enableWrite = !enableRendering;

    if (enableRendering)
    {
        // Copy bits related to horizontal position
        constexpr c6502_word_t CPYMSK = 0b000010000011111u;
        m_st.vramAddr &= ~CPYMSK;
        m_st.vramAddr |= m_st.tmpAddr & CPYMSK;
    }

    // Render background
    if (!NTSCLineSkip)
    {
        if (m_st.backgroundVisible)
        {
            for (int c = 0; c < 33; c++)
            {
                if (m_st.fullBacgroundVisible || c > 0)
                {
                    // Read character index from character area
                    const c6502_word_t charAddr = 0x2000u | (m_st.vramAddr & 0x0FFFu);
                    const auto charNum = bus().readVideoMem(charAddr);

                    // Read color information from attribute area
                    const c6502_word_t attrAddr = 0x23C0u |
                                                (m_st.vramAddr & 0b110000000000u) |
                                                ((m_st.vramAddr >> 4u) & 0b111000u) |
                                                ((m_st.vramAddr >> 2u) & 0b111u);
                    const auto clrGrp = bus().readVideoMem(attrAddr);
                    const auto offInGrp = ((m_st.vramAddr >> 5u) & 0b10u) |
                                        ((m_st.vramAddr >> 1u) & 0b01u);
                    const c6502_byte_t clrHi = (clrGrp >> offInGrp * 2u) & 0b11u;

                    // Load character / attribute data
                    const c6502_word_t x = c * 8u;
                    assert(x <= 256u);
                    readCharacterLine(lnData + x, charNum, fineY, m_st.baBkgnd, false, false);
                    expandColor(lnData + x, clrHi, PAL_BG);
                }

                m_st.vramAddr = incrWrpAddrHorz(m_st.vramAddr);
            }
        }

        // Render sprites
        if (m_st.spritesVisible)
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
                    assert(x + fineX <= 256 + 8);
                    auto &bp = lnData[x + fineX + i],
                        &sp = sprLnData[i];
                    if (sp != TRANSPARENT_PXL)
                    {
                        // Sprite 0 hit test
                        if (ns == 0 && bp != TRANSPARENT_PXL && x < 255u)
                            m_st.sprite0 = true;

                        if (!behindBg || bp == TRANSPARENT_PXL)
                            bp = sp;
                    }
                }

                nSprites++;
            }
            if (nSprites > 8)
                m_st.over8sprites = true;
        }
    }

    if (enableRendering)
        m_st.vramAddr = incrWrpAddrVert(m_st.vramAddr);

    if (!NTSCLineSkip)
    {
        assert(m_pBackend != nullptr);
        m_pBackend->setLine(m_currLine, lnData + fineX, bus().readVideoMem(0x3F00u));
    }

    m_currLine++;
}

void PPU::endFrame() noexcept
{
    assert(m_pBackend != nullptr);
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
        *p = *p > 0 ? bus().readVideoMem(palAddr + (*p | clrHi)) : TRANSPARENT_PXL;
}

void writeBool(std::ostream &out, bool v)
{
    out.put(v ? 1 : 0);
}

void writeWord(std::ostream &out, c6502_word_t v)
{
    out.put(lo_byte(v));
    out.put(hi_byte(v));
}

void writeByte(std::ostream &out, c6502_byte_t v)
{
    out.put(v);
}

bool readBool(std::istream &in)
{
    std::istream::char_type t;
    in.get(t);
    return t == 1;
}

c6502_word_t readWord(std::istream &in)
{
    std::istream::char_type t[2];
    in.read(t, 2);
    return combine(t[0], t[1]);
}

c6502_byte_t readByte(std::istream &in)
{
    std::istream::char_type t;
    in.get(t);
    return t;
}

size_t PPU::saveState(std::ostream &out)
{
    const auto s = out.tellp();

    writeBool(out, m_st.enableNMI);
    writeBool(out, m_st.bigSprites);
    writeBool(out, m_st.spritesVisible);
    writeBool(out, m_st.backgroundVisible);
    writeBool(out, m_st.allSpritesVisible);
    writeBool(out, m_st.fullBacgroundVisible);
    writeBool(out, m_st.vblank);
    writeBool(out, m_st.sprite0);
    writeBool(out, m_st.enableWrite);
    writeBool(out, m_st.over8sprites);
    writeWord(out, m_st.baBkgnd);
    writeWord(out, m_st.baSprites);
    writeWord(out, m_st.addrIncr);
    writeWord(out, m_st.vramAddr);
    writeWord(out, m_st.tmpAddr);
    writeWord(out, m_st.fineX);
    writeWord(out, m_st.sprmemAddr);
    writeByte(out, m_st.vramReadBuf);
    writeByte(out, m_st.w);
    writeByte(out, m_currLine);

    const auto len = out.tellp() - s;
    assert(len == 27);

    return 27;
}

size_t PPU::loadState(std::istream &in)
{
    const auto s = in.tellg();

    m_st.enableNMI = readBool(in);
    m_st.bigSprites = readBool(in);
    m_st.spritesVisible = readBool(in);
    m_st.backgroundVisible = readBool(in);
    m_st.allSpritesVisible = readBool(in);
    m_st.fullBacgroundVisible = readBool(in);
    m_st.vblank = readBool(in);
    m_st.sprite0 = readBool(in);
    m_st.enableWrite = readBool(in);
    m_st.over8sprites = readBool(in);
    m_st.baBkgnd = readWord(in);
    m_st.baSprites = readWord(in);
    m_st.addrIncr = readWord(in);
    m_st.vramAddr = readWord(in);
    m_st.tmpAddr = readWord(in);
    m_st.fineX = readWord(in);
    m_st.sprmemAddr = readWord(in);
    m_st.vramReadBuf = readByte(in);
    m_st.w = readByte(in);
    m_currLine = readByte(in);

    const auto len = in.tellg() - s;
    assert(len == 27);

    return 27;
}
