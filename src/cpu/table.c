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

#include <stdint.h>
#include <stddef.h>

#include "table.h"
#include "cpu.h"
#include "logic.h"
#include "cpu_types.h"
#include <stdint.h>

#include "table.h"
#include "cpu.h"
#include "logic.h"

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
 *  - fetch opcode
 *  - return handler
 */
x86_fn_t x86_decode_ctx(exec_ctx_t *e)
{
    uint8_t op = 0;

    // PEEK opcode at CS:IP (do NOT advance IP here)
    uint16_t w = 0;
    if (!x86_read16(e, e->cpu->cs, e->cpu->ip, &w)) return op_unknown;
    op = (uint8_t)(w & 0xFF);

    // Minimal “switch decoder” (good enough until the table is populated)
    if (op == 0x83) return handle_grp1_83;

    if (op >= 0xB8 && op <= 0xBF) return op_mov_r16_imm16;

    switch (op) {
        case 0x90: return op_nop;
        case 0xF4: return op_hlt;
        default:   return op_unknown;
    }
}