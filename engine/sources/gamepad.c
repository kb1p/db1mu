#include "gamepad.h"

static c6502_byte_t psg[...];

static int joy_1, joy_readbit;

c6502_byte_t gp_read_joy1()
{
    c6502_byte_t ret = (joy_1 >> joy_readbit) & 1;
    joy_readbit = (joy_readbit + 1) & 7;
    return ret;
}

void gp_write_joy1(c6502_byte_t v)
{
    if ((psg[0x16] & 1) && !(v & 1))
    {
        joy_readbit = 0;
        keyscan();
    }
    psg[0x16] = v;
}
