#ifndef APU_H
#define APU_H

#include "common.h"

class PlaybackBackend
{
};

class APU: public Component
{
public:
    enum Register: c6502_word_t
    {
        RCT1_CTRL    = 0x0u,
        RCT1_GEN     = 0x1u,
        RCT1_FREQ1   = 0x2u,
        RCT1_FREQ2   = 0x3u,
        RCT2_CTRL    = 0x4u,
        RCT2_GEN     = 0x5u,
        RCT2_FREQ1   = 0x6u,
        RCT2_FREQ2   = 0x7u,
        TRI_CTRL     = 0x8u,
        TRI_FREQ1    = 0xAu,
        TRI_FREQ2    = 0xBu,
        NOIS_CTRL1   = 0xCu,
        NOIS_CTRL2   = 0xEu,
        NOIS_FREQ    = 0xFu,
        CTRL_STATUS  = 0x15u,
        FRAME_SEQ    = 0x17u
    };

    c6502_byte_t readRegister(c6502_word_t reg);
    void writeRegister(c6502_word_t reg, c6502_byte_t val);
};

#endif
