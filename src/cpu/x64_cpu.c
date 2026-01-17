// src/x64_cpu.c

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
#include <stdio.h>

#include "x64_cpu.h"
#include "cpu.h"
#include "cpu_system.h"
#include "execute.h"

// NOTE: exec_ctx.h / logic.h were previously included for an external handler path,
// but this file currently implements decode/execute inline. Keeping those headers
// caused signature mismatches and/or unused-context churn, so they are intentionally
// not included here.

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

static void trace_fetch_win(x86_cpu_t *c) {
    uint8_t b[TRACE_WIN_BYTES];
    uint16_t cs = c->cs;
    uint16_t ip = c->ip;

    uint32_t base = x86_linear_addr(cs, ip);

    for (int i = 0; i < TRACE_WIN_BYTES; i++) {
        uint8_t v = 0;
        if (!mem_read8(c, base + (uint32_t)i, &v)) {
            b[i] = 0x00;
        } else {
            b[i] = v;
        }
    }


    fprintf(stderr, "%04X:%04X  ", cs, ip);
    for (int i = 0; i < TRACE_WIN_BYTES; i++) {
        fprintf(stderr, "%02X%s", b[i], (i == TRACE_WIN_BYTES - 1) ? "" : " ");
    }
    fprintf(stderr, "\n");
}


x86_status_t x86_step(x86_cpu_t *c) {
    if (c->halted) return X86_HALT;

    // Prefix handling: only REP (F3) for now. We store into c->rep_prefix
    // to avoid changing your CPU struct, but we clear it each step so it
    // never becomes "sticky" across instructions.
    c->rep_prefix = false;

    uint8_t op = 0;
    for (;;) {
        if (!fetch8(c, &op)) {
            fprintf(stderr, "x86_step: fetch fault at CS:IP %04x:%04x\n", c->cs, (uint16_t)(c->ip - 1));
            return X86_ERR;
        }
        if (op == 0xF3) { c->rep_prefix = true; continue; } // REP/REPE
        break;
    }
	
	if (c->halted) return X86_HALT;

	trace_fetch_win(c);

    exec_ctx_t e = {
        .cpu = c,
    };

//    fprintf(stderr, "x86_step: unhandled opcode 0x%02X at CS:IP %04x:%04x\n",
//            op, c->cs, (uint16_t)(c->ip - 1));

    return cpu_execute(&e);
}
