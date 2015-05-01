/*
 * 6502 CPU emulation routines.
 * 
 * TODO: replace all these magic numbers with #defines at least.
 */

#include "cpu6502.h"

/* Layout:
 * - accumulator
 * - flags
 * - X, Y indexes
 * - stack pointer
 * - program counter
 */
static struct c6502_reg
{
    c6502_byte_t a, p, x, y, s;
    union
    {
        struct
        {
            c6502_byte_t l, h;
        } B;
        c6502_word_t W;
    } pc;
} cpu_regs;

static c6502_mode_t cpu_mode = PAL;
static c6502_state_t cpu_state = STATE_HALTED;
static int iPeriod, iTact;

// Tacts per period: { PAL, NTSC }
static const int tpp[2] = { 59182, 71595 };

/*** 6502 MEMORY MAP ***/
// Internal RAM: 0x0000 ~ 0x2000. 
// 0x0000 ~ 0x0100 is a z-page, have special meaning for addressing.
static c6502_byte_t ram[0x800];

// Cartridge RAM: 0x6000 ~ 0x8000
static c6502_byte_t wram[0x2000];

// Catridge ROM may contain from 32 to 256kb of 2kb-sized pages
// Basic addressed are 0x8000 ~ 0xFFFF; addresses 0x8000 ~ 0xC000 are a switchable banks
static c6502_byte_t rom[128][0x2000];

// Num. cycles per 6502 instruction
static const c6502_byte_t cycles[256] =
{
    /*0x00*/ 7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
    /*0x10*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x20*/ 6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
    /*0x30*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x40*/ 6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
    /*0x50*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x60*/ 6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
    /*0x70*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x80*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /*0x90*/ 2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
    /*0xA0*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /*0xB0*/ 2,5,2,5,4,4,4,4,2,4,2,4,4,4,4,4,
    /*0xC0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /*0xD0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0xE0*/ 2,6,3,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /*0xF0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
};

void c6502_set_mode(c6502_mode_t cm)
{
    cpu_mode = cm;
}

c6502_byte_t c6502_memory_read(c6502_word_t addr)
{
    switch (addr >> 13)
    {
        case 0:
            return ram[addr & 0x7FF];
        case 3:
            return wram[addr & 0x1FFF];
        case 1:
        case 2:
            return c6502_io_read(addr);
        default:
            addr &= 0x7FFF;
            return rom[addr >> 13][addr];
    }
}

void c6502_memory_write(c6502_word_t addr, c6502_byte_t val)
{
    switch (addr >> 13)
    {
        case 0:
            // To internal RAM
            ram[addr & 0x7FF] = val;
            break;
        case 3:
            // To cartridge RAM
            wram[addr & 0x1FFF] = val;
            break;
        case 1:
        case 2:
            // To GPU or APU registers
            c6502_io_write(addr, val);
            break;
        default:
            // Write to registers of memory controller on cartridge
            c6502_mmc_write(addr, val);
    }
}

void c6502_update_screen()
{
}

void c6502_test_keys()
{
}

void c6502_reset()
{
    cpu_regs.a = cpu_regs.x = cpu_regs.y = 0;
    cpu_regs.p = 0x22;
    cpu_regs.s = 0xFF;
    cpu_regs.pc.B.l = c6502_memory_read(0xFFFC);
    cpu_regs.pc.B.h = c6502_memory_read(0xFFFD);
    iPeriod = tpp[cpu_mode];
}

// Handle non-maskable interrupt
void c6502_nmi()
{
    c6502_memory_write(0x100 | cpu_regs.s, cpu_regs.pc.B.h);
    cpu_regs.s--;
    c6502_memory_write(0x100 | cpu_regs.s, cpu_regs.pc.B.l);
    cpu_regs.s--;
    c6502_memory_write(0x100 | cpu_regs.s, cpu_regs.p);
    cpu_regs.s--;
    
    cpu_regs.pc.B.l = c6502_memory_read(0xFFFA);
    cpu_regs.pc.B.h = c6502_memory_read(0xFFFB);
    
    iPeriod -= 7;
}

// Handle maskable interrupt
void c6502_irq()
{
}

void c6502_run()
{
    c6502_reset();
    while (cpu_state == STATE_RUN)
    {
        iTact = c6502_step();
        iPeriod -= iTact;
        if (iPeriod <= 0)
        {
            iPeriod += tpp[cpu_mode];
            c6502_update_screen();
            c6502_test_keys();
            c6502_nmi();
        }
    }
}

c6502_byte_t c6502_step()
{
    c6502_byte_t opcode = c6502_memory_read(cpu_regs.pc.W++);
    
    // Emulate all opcodes
    switch (opcode)
    {
        #include "opjob.inc"
        default:
            cpu_state = STATE_ERROR;

            // TODO: add error handling
            return 0;
    }

    return cycles[opcode];
}

