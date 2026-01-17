// src/cpu/cpu.c

/*
 * Copyright 2025 Thomas L Hamilton
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cpu.h"

// --------------------------

static uint16_t *reg16_by_index(x86_cpu_t *c, unsigned idx);

// -------------------------

void x86_init(x86_cpu_t *c, uint8_t *mem, size_t mem_size) {
    *c = (x86_cpu_t){0};
    c->mem = mem;
    c->mem_size = mem_size;

    // Reasonable 16-bit reset-ish defaults for now
    c->ss = 0x0000;
    c->sp = 0xFFFE;
    c->flags = 0x0002; // bit 1 is typically always set on 8086/real-mode-ish
}

uint16_t* reg16_by_index(x86_cpu_t *c, unsigned idx) {
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

/* =======================
 * exec_ctx helper layer
 * ======================= */

bool x86_fetch8(exec_ctx_t *e, uint8_t *out)
{
    uint32_t a = x86_linear_addr(e->cpu->cs, e->cpu->ip);
    if (!mem_read8(e->cpu, a, out)) return false;
    e->cpu->ip++;
    return true;
}

bool x86_fetch16(exec_ctx_t *e, uint16_t *out)
{
    uint32_t a = x86_linear_addr(e->cpu->cs, e->cpu->ip);
    if (!mem_read16(e->cpu, a, out)) return false;
    e->cpu->ip = (uint16_t)(e->cpu->ip + 2);
    return true;
}

uint16_t *x86_reg16(exec_ctx_t *e, unsigned reg)
{
    return reg16_by_index(e->cpu, reg);
}

bool x86_read16(exec_ctx_t *e, uint16_t seg, uint16_t off, uint16_t *out)
{
    uint32_t a = x86_linear_addr(seg, off);
    return mem_read16(e->cpu, a, out);
}

bool x86_write16(exec_ctx_t *e, uint16_t seg, uint16_t off, uint16_t val)
{
    uint32_t a = x86_linear_addr(seg, off);
    return mem_write16(e->cpu, a, val);
}

void x86_set_cf(exec_ctx_t *e, bool v)
{
    if (v) e->cpu->flags |= X86_FL_CF;
    else   e->cpu->flags &= ~X86_FL_CF;
}

void x86_set_zf_sf16(exec_ctx_t *e, uint16_t r)
{
    if (r == 0) e->cpu->flags |= X86_FL_ZF;
    else        e->cpu->flags &= ~X86_FL_ZF;

    if (r & 0x8000u) e->cpu->flags |= X86_FL_SF;
    else             e->cpu->flags &= ~X86_FL_SF;
}