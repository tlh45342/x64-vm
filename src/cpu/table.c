// src/cpu/table.c

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

// --- tiny handlers (keep local) ---
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cpu/table.h"
#include "cpu/memops.h"
#include "cpu/x86_cpu.h"
#include "cpu/logic.h"
#include "cpu/cpu_types.h"
#include "cpu/exec_ctx.h"

#include "vm/vm.h"   // vm_read8()

// PEEK a byte at seg:off without advancing IP.
static bool peek8(exec_ctx_t *e, uint16_t seg, uint16_t off, uint8_t *out)
{
    if (!e || !e->cpu || !out) return false;

    const uint32_t lin = x86_linear_addr(seg, off);

    // Preferred path: VM owns memory.
    if (e->vm) {
        return vm_read8(e->vm, lin, out);
    }

    // Fallback: transitional CPU-backed memory.
    x86_cpu_t *c = e->cpu;
    if (!c->mem) return false;
    if (lin >= (uint32_t)c->mem_size) return false;

    *out = c->mem[lin];
    return true;
}

// --- tiny handlers (keep local) ---

static x86_status_t op_unknown(exec_ctx_t *e) {
    uint8_t op = 0;
    (void)x86_fetch8(e, &op);   // consume 1 byte so we don't spin forever
    return X86_ILLEGAL;
}

static x86_status_t op_nop(exec_ctx_t *e) {
    uint8_t op = 0;
    if (!x86_fetch8(e, &op)) return X86_FAULT;  // consume 0x90
    return X86_OK;
}

static x86_status_t op_hlt(exec_ctx_t *e) {
    uint8_t op = 0;
    if (!x86_fetch8(e, &op)) return X86_FAULT;  // consume 0xF4
    return X86_HALT;
}

/*
 * Minimal decoder:
 *  - peek opcode (do not advance IP)
 *  - return handler
 */
x86_fn_t x86_decode_ctx(exec_ctx_t *e)
{
    uint8_t op = 0;

    // PEEK opcode at CS:IP (do NOT advance IP here)
    if (!peek8(e, e->cpu->cs, e->cpu->ip, &op)) return op_unknown;

    // Minimal “switch decoder” (good enough until the table is populated)
    if (op == 0x83) return handle_grp1_83;

    if (op >= 0xB8 && op <= 0xBF) return op_mov_r16_imm16;

    switch (op) {
        case 0x90: return op_nop;
        case 0xF4: return op_hlt;
        default:   return op_unknown;
    }
}