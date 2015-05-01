#ifndef CPU6502_H
#define CPU6502_H

#include "common.h"

typedef enum
{
    PAL = 0, NTSC
} c6502_mode_t;

typedef enum
{
    STATE_HALTED, 
    STATE_RUN, 
    STATE_ERROR
} c6502_state_t;

// 6502 opcodes
#define ADC_IMM 0x69
#define ADC_ZP  0x65
#define ADC_ZPX 0x75
#define ADC_ABS 0x6D
#define ADC_ABX 0x7D
#define ADC_ABY 0x79
#define ADC_INX 0x61
#define ADC_INY 0x71
#define AND_IMM 0x29
#define AND_ZP  0x25
#define AND_ZPX 0x35
#define AND_ABS 0x2D
#define AND_ABX 0x3D
#define AND_ABY 0x39
#define AND_INX 0x21
#define AND_INY 0x31
#define ASL_ACC 0x0A
#define ASL_ZP  0x06
#define ASL_ZPX 0x16
#define ASL_ABS 0x0E
#define ASL_ABX 0x1E
#define BCC     0x90
#define BCS     0xB0
#define BEQ     0xF0
#define BIT_ZP  0x24
#define BIT_ABS 0x2C
#define BMI     0x30
#define BNE     0xD0
#define BPL     0x10
#define BRK     0x00
#define BVC     0x50
#define BVS     0x70
#define CLC     0x18
#define CLD     0xD8
#define CLI     0x58
#define CLV     0xB8
#define CMP_IMM 0xC9
#define CMP_ZP  0xC5
#define CMP_ZPX 0xD5
#define CMP_ABS 0xCD
#define CMP_ABX 0xDD
#define CMP_ABY 0xD9
#define CMP_INX 0xC1
#define CMP_INY 0xD1
#define CPX_IMM 0xE0
#define CPX_ZP  0xE4
#define CPX_ABS 0xEC
#define CPY_IMM 0xC0
#define CPY_ZP  0xC4
#define CPY_ABS 0xCC
#define DEC_ZP  0xC6
#define DEC_ZPX 0xD6
#define DEC_ABS 0xCE
#define DEC_ABX 0xDE
#define DEX     0xCA
#define DEY     0x88
#define EOR_IMM 0x49
#define EOR_ZP  0x45
#define EOR_ZPX 0x55
#define EOR_ABS 0x4D
#define EOR_ABX 0x5D
#define EOR_ABY 0x59
#define EOR_INX 0x41
#define EOR_INY 0x51
#define INC_ZP  0xE6
#define INC_ZPX 0xF6
#define INC_ABS 0xEE
#define INC_ABX 0xFE
#define INX     0xE8
#define INY     0xC8
#define JMP_ABS 0x4C
#define JMP_IND 0x6C
#define JSR     0x20
#define LDA_IMM 0xA9
#define LDA_ZP  0xA5
#define LDA_ZPX 0xB5
#define LDA_ABS 0xAD
#define LDA_ABX 0xBD
#define LDA_ABY 0xB9
#define LDA_INX 0xA1
#define LDA_INY 0xB1
#define LDX_IMM 0xA2
#define LDX_ZP  0xA6
#define LDX_ZPY 0xB6
#define LDX_ABS 0xAE
#define LDX_ABY 0xBE
#define LDY_IMM 0xA0
#define LDY_ZP  0xA4
#define LDY_ZPY 0xB4
#define LDY_ABS 0xAC
#define LDY_ABY 0xBC
#define LSR_ACC 0x4A

#ifdef __cplusplus
extern "C" {
#endif
    
void c6502_set_mode(c6502_mode_t cm);
c6502_byte_t c6502_memory_read(c6502_word_t addr);
void c6502_memory_write(c6502_word_t addr, c6502_byte_t val);
c6502_byte_t c6502_io_read(c6502_word_t addr);
void c6502_io_write(c6502_word_t addr, c6502_byte_t val);
c6502_byte_t c6502_mmc_read(c6502_word_t addr);
void c6502_mmc_write(c6502_word_t addr, c6502_byte_t val);
void c6502_update_screen();
void c6502_test_keys();
void c6502_nmi();
void c6502_irq();
c6502_byte_t c6502_step();
void c6502_reset();
void c6502_run();

#ifdef __cplusplus
}
#endif

#endif