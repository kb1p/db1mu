#ifndef B1MULATOR_DEBUGCOMMAND_H
#define B1MULATOR_DEBUGCOMMAND_H

#include "common.h"

struct DebugCommand {
    enum CMD {
        CMD_PrintMemoryByte,
        CMD_PrintMemoryArray,
        CMD_PrintCPUState,
        CMD_Continue,
        CMD_Break,
        CMD_RST,
        CMD_Unknown
    };
    CMD cmd;

    union {
        struct {c6502_word_t mem_ptr; c6502_word_t array_len; } mem_array;
        c6502_word_t mem_byte;
        c6502_word_t break_addr;
    } args;
};

#endif //B1MULATOR_DEBUGCOMMAND_H
