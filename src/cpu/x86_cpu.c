// src/cpu/x86_cpu.c

/*
 * Copyright 2026 Thomas L Hamilton
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

#include "x86_cpu.h"
#include "exec_ctx.h"
#include "execute.h"

typedef struct x86_decoded {
    x86_cpu_t *c;

    uint16_t start_cs;
    uint16_t start_ip;

    // prefixes (start small)
    bool rep;          // F3

    // opcode
    uint8_t op;

    // decoded fields (for now)
    uint8_t reg;       // generic reg field used by some ops
    uint16_t imm16;

    // length bookkeeping (optional now, useful soon)
    uint8_t ilen;
} x86_decoded_t;

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

uint16_t *x86_reg16(exec_ctx_t *e, unsigned reg)
{
    return reg16_by_index(e->cpu, reg);
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

/* Keep the canonical linear addr helper here unless you move it elsewhere */
uint32_t x86_linear_addr(uint16_t seg, uint16_t off)
{
    return ((uint32_t)seg << 4) + (uint32_t)off;
}

// NOTE: exec_ctx.h / logic.h were previously included for an external handler path,
// but this file currently implements decode/execute inline. Keeping those headers
// caused signature mismatches and/or unused-context churn, so they are intentionally
// not included here.


typedef x86_status_t (*x86_op_fn)(x86_decoded_t *d);

typedef struct x86_opent {
    x86_op_fn fn;
    uint8_t imm_bytes;   // 0,1,2,4
    uint8_t flags;       // future: needs_modrm, etc.
} x86_opent_t;

static inline uint32_t rm_phys16(uint16_t seg, uint16_t off) {
    return ((uint32_t)seg << 4) + off;
}

#ifndef TRACE_WIN_BYTES
#define TRACE_WIN_BYTES 16   // change to 8 if you want
#endif


x86_status_t x86_step(exec_ctx_t *e)
{
    if (!e || !e->cpu) return X86_ERR;
    if (e->cpu->halted) return X86_HALT;
    return cpu_execute(e);
}

