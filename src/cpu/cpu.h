#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct x86_cpu {
    // general purpose regs (16-bit real mode for now)
    uint16_t ax, bx, cx, dx;
    uint16_t si, di, bp, sp;

    // segment regs
    uint16_t cs, ds, es, ss;

    // instruction pointer
    uint16_t ip;

    // flags register
    uint16_t flags;

    // memory backing
    uint8_t  *mem;
    size_t    mem_size;

    bool halted;
} x86_cpu_t;

// flags bits (add as needed)
enum {
    X86_FL_CF = 1u << 0,
    X86_FL_PF = 1u << 2,
    X86_FL_AF = 1u << 4,
    X86_FL_ZF = 1u << 6,
    X86_FL_SF = 1u << 7,
    X86_FL_TF = 1u << 8,
    X86_FL_IF = 1u << 9,
    X86_FL_DF = 1u << 10,
    X86_FL_OF = 1u << 11,
};

void     x86_init(x86_cpu_t *c, uint8_t *mem, size_t mem_size);
uint32_t x86_linear_addr(uint16_t seg, uint16_t off);

// These are used across modules; don’t hide them as static forever.
uint8_t  *reg8_by_index (x86_cpu_t *c, unsigned idx);
uint16_t *reg16_by_index(x86_cpu_t *c, unsigned idx);

// memory helpers (implemented in decode.c for now)
bool mem_read8 (x86_cpu_t *c, uint32_t addr, uint8_t  *out);
bool mem_read16(x86_cpu_t *c, uint32_t addr, uint16_t *out);
bool mem_write8 (x86_cpu_t *c, uint32_t addr, uint8_t  val);
bool mem_write16(x86_cpu_t *c, uint32_t addr, uint16_t val);

// fetch helpers (implemented in decode.c for now)
bool fetch8 (x86_cpu_t *c, uint8_t  *out);
bool fetch16(x86_cpu_t *c, uint16_t *out);

// stack helpers (implemented in cpu.c)
bool push16(x86_cpu_t *c, uint16_t val);
bool pop16 (x86_cpu_t *c, uint16_t *out);