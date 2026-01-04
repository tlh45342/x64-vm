// src/cpu/cpu.c

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cpu.h"

void x86_init(x86_cpu_t *c, uint8_t *mem, size_t mem_size) {
    *c = (x86_cpu_t){0};
    c->mem = mem;
    c->mem_size = mem_size;

    // Reasonable 16-bit reset-ish defaults for now
    c->ss = 0x0000;
    c->sp = 0xFFFE;
    c->flags = 0x0002; // bit 1 is typically always set on 8086/real-mode-ish
}

uint32_t x86_linear_addr(uint16_t seg, uint16_t off) {
    // Real-mode physical address = seg*16 + off
    // (A20 wrap behavior can be added later)
    return ((uint32_t)seg << 4) + (uint32_t)off;
}

static uint8_t *reg8_by_index(x86_cpu_t *c, unsigned idx) {
    // idx: 0=AL 1=CL 2=DL 3=BL 4=AH 5=CH 6=DH 7=BH
    switch (idx & 7u) {
        case 0: return (uint8_t *)&c->ax;           // AL
        case 1: return (uint8_t *)&c->cx;           // CL
        case 2: return (uint8_t *)&c->dx;           // DL
        case 3: return (uint8_t *)&c->bx;           // BL
        case 4: return ((uint8_t *)&c->ax) + 1;     // AH
        case 5: return ((uint8_t *)&c->cx) + 1;     // CH
        case 6: return ((uint8_t *)&c->dx) + 1;     // DH
        case 7: return ((uint8_t *)&c->bx) + 1;     // BH
    }
    return (uint8_t *)&c->ax;
}

static uint16_t* reg16_by_index(x86_cpu_t *c, unsigned idx) {
    // idx from opcode B8+rw: 0=AX,1=CX,2=DX,3=BX,4=SP,5=BP,6=SI,7=DI
    switch (idx & 7u) {
        case 0: return &c->ax;
        case 1: return &c->cx;
        case 2: return &c->dx;
        case 3: return &c->bx;
        case 4: return &c->sp;
        case 5: return &c->bp;
        case 6: return &c->si;
        case 7: return &c->di;
        default: return &c->ax;
    }
}

// 8086-style push: SP -= 2; [SS:SP] = val
static bool push16(x86_cpu_t *c, uint16_t val) {
    c->sp = (uint16_t)(c->sp - 2);
    uint32_t a = x86_linear_addr(c->ss, c->sp);
    return mem_write16(c, a, val);
}

// 8086-style pop: val = [SS:SP]; SP += 2
static bool pop16(x86_cpu_t *c, uint16_t *out) {
    uint32_t a = x86_linear_addr(c->ss, c->sp);
    if (!mem_read16(c, a, out)) return false;
    c->sp = (uint16_t)(c->sp + 2);
    return true;
}