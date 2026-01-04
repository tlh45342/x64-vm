// src/cpu_system.c
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "cpu_system.h"

// Local helpers (kept private to this module)
static uint32_t x86_linear_addr(uint16_t seg, uint16_t off) {
    return ((uint32_t)seg << 4) + (uint32_t)off;
}

static bool mem_read8(x86_cpu_t *c, uint32_t addr, uint8_t *out) {
    if (addr >= c->mem_size) return false;
    *out = c->mem[addr];
    return true;
}

static bool mem_read16(x86_cpu_t *c, uint32_t addr, uint16_t *out) {
    uint8_t lo, hi;
    if (!mem_read8(c, addr, &lo)) return false;
    if (!mem_read8(c, addr + 1, &hi)) return false;
    *out = (uint16_t)(lo | ((uint16_t)hi << 8));
    return true;
}

static bool mem_write8(x86_cpu_t *c, uint32_t addr, uint8_t val) {
    if (addr >= c->mem_size) return false;
    c->mem[addr] = val;
    return true;
}

static bool mem_write16(x86_cpu_t *c, uint32_t addr, uint16_t val) {
    if (!mem_write8(c, addr, (uint8_t)(val & 0x00FFu))) return false;
    if (!mem_write8(c, addr + 1, (uint8_t)((val >> 8) & 0x00FFu))) return false;
    return true;
}

static bool fetch8(x86_cpu_t *c, uint8_t *out) {
    uint32_t a = x86_linear_addr(c->cs, c->ip);
    if (!mem_read8(c, a, out)) return false;
    c->ip++;
    return true;
}

// 8086-style push: SP -= 2; [SS:SP] = val
static bool push16(x86_cpu_t *c, uint16_t val) {
    c->sp = (uint16_t)(c->sp - 2);
    uint32_t a = x86_linear_addr(c->ss, c->sp);
    return mem_write16(c, a, val);
}

// IVT entry n at physical 0x0000: (offset @ 4n, segment @ 4n+2)
static bool ivt_get_vector(x86_cpu_t *c, uint8_t n, uint16_t *out_ip, uint16_t *out_cs) {
    uint32_t base = (uint32_t)n * 4u;
    uint16_t ip, cs;
    if (!mem_read16(c, base + 0, &ip)) return false;
    if (!mem_read16(c, base + 2, &cs)) return false;
    *out_ip = ip;
    *out_cs = cs;
    return true;
}

x86_status_t handle_int_cd(x86_cpu_t *c) {
    uint8_t n = 0;
    if (!fetch8(c, &n)) return X86_ERR;

    // Push FLAGS, CS, IP (IP already points to next instruction after imm8)
    if (!push16(c, c->flags)) return X86_ERR;
    if (!push16(c, c->cs))    return X86_ERR;
    if (!push16(c, c->ip))    return X86_ERR;

    // Clear IF and TF (bits 9 and 8)
    c->flags = (uint16_t)(c->flags & ~(uint16_t)0x0300u);

    uint16_t new_ip = 0, new_cs = 0;
    if (!ivt_get_vector(c, n, &new_ip, &new_cs)) return X86_ERR;

    c->cs = new_cs;
    c->ip = new_ip;
    return X86_OK;
}