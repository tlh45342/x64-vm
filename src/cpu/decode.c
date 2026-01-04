// src/cpu/decode.c

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"

uint16_t get_sreg(x86_cpu_t *c, unsigned s) {
    switch (s & 3u) {
        case 0: return c->es;
        case 1: return c->cs;
        case 2: return c->ss;
        case 3: return c->ds;
    }
    return c->ds;
}

void set_sreg(x86_cpu_t *c, unsigned s, uint16_t v) {
    switch (s & 3u) {
        case 0: c->es = v; break;
        case 1: c->cs = v; break;
        case 2: c->ss = v; break;
        case 3: c->ds = v; break;
    }
}

bool ea16_compute(x86_cpu_t *c, uint8_t modrm, uint16_t *out_ea) {
    uint8_t mod = (modrm >> 6) & 3u;
    uint8_t rm  = (modrm >> 0) & 7u;

    int16_t disp = 0;
    if (mod == 0u) {
        if (rm == 6u) {
            // [disp16]
            uint16_t d16 = 0;
            if (!fetch16(c, &d16)) return false;
            disp = (int16_t)d16;
            *out_ea = (uint16_t)disp;
            return true;
        }
    } else if (mod == 1u) {
        // disp8 sign-extended
        uint8_t d8 = 0;
        if (!fetch8(c, &d8)) return false;
        disp = (int8_t)d8;
    } else if (mod == 2u) {
        // disp16
        uint16_t d16 = 0;
        if (!fetch16(c, &d16)) return false;
        disp = (int16_t)d16;
    } else {
        // mod==3 is register, not memory
        return false;
    }

    uint16_t base = 0;
    switch (rm) {
        case 0: base = (uint16_t)(c->bx + c->si); break; // [BX+SI]
        case 1: base = (uint16_t)(c->bx + c->di); break; // [BX+DI]
        case 2: base = (uint16_t)(c->bp + c->si); break; // [BP+SI]
        case 3: base = (uint16_t)(c->bp + c->di); break; // [BP+DI]
        case 4: base = c->si; break;                     // [SI]
        case 5: base = c->di; break;                     // [DI]
        case 6: base = c->bp; break;                     // [BP]  (mod!=0)
        case 7: base = c->bx; break;                     // [BX]
    }

    *out_ea = (uint16_t)(base + disp);
    return true;
}

bool mem_read8(x86_cpu_t *c, uint32_t addr, uint8_t *out) {
    if (addr >= c->mem_size) return false;
    *out = c->mem[addr];
    return true;
}

bool mem_read16(x86_cpu_t *c, uint32_t addr, uint16_t *out) {
    uint8_t lo, hi;
    if (!mem_read8(c, addr, &lo)) return false;
    if (!mem_read8(c, addr + 1, &hi)) return false;
    *out = (uint16_t)(lo | ((uint16_t)hi << 8));
    return true;
}

bool mem_write8(x86_cpu_t *c, uint32_t addr, uint8_t val) {
    if (addr >= c->mem_size) return false;
    c->mem[addr] = val;
    return true;
}

bool mem_write16(x86_cpu_t *c, uint32_t addr, uint16_t val) {
    if (!mem_write8(c, addr, (uint8_t)(val & 0x00FFu))) return false;
    if (!mem_write8(c, addr + 1, (uint8_t)((val >> 8) & 0x00FFu))) return false;
    return true;
}

bool fetch8(x86_cpu_t *c, uint8_t *out) {
    uint32_t a = x86_linear_addr(c->cs, c->ip);
    if (!mem_read8(c, a, out)) return false;
    c->ip++;
    return true;
}

bool fetch16(x86_cpu_t *c, uint16_t *out) {
    uint32_t a = x86_linear_addr(c->cs, c->ip);
    if (!mem_read16(c, a, out)) return false;
    c->ip += 2;
    return true;
}