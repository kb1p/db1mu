#ifndef GAMEPAD_H
#define GAMEPAD_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif
    
void gp_write_joy1(c6502_byte_t v);

c6502_byte_t gp_read_joy1();
    
#ifdef __cplusplus
}
#endif

#endif