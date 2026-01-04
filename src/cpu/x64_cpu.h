// src/x64_cpu.h

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct x86_cpu {
    // 16-bit real-mode registers for now
    uint16_t ax, bx, cx, dx;
    uint16_t sp, bp, si, di;

    uint16_t cs, ds, es, ss;
    uint16_t ip;
    uint16_t flags; // not used yet

    bool halted;
	bool rep_prefix;   // set when 0xF3 seen, consumed by next string instruction
    uint8_t *mem;
    size_t mem_size;
} x86_cpu_t;

typedef enum {
    X86_OK = 0,
    X86_HALT = 1,
    X86_ERR = -1
} x86_status_t;

void x86_init(x86_cpu_t *c, uint8_t *mem, size_t mem_size);
uint32_t x86_linear_addr(uint16_t seg, uint16_t off);
x86_status_t x86_step(x86_cpu_t *c);