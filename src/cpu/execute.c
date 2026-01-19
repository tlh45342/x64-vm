// src/cpu/execute.c

/*
 * Copyright 2025-2026 Thomas L Hamilton
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

#include "cpu_types.h"
#include "exec_ctx.h"
#include "execute.h"
#include "x86_cpu.h"
#include "cpu/disasm.h"
#include "util/log.h"
#include "cpu/table.h"   // x86_decode_ctx(), x86_exec_fn_t
#include "vm/vm.h"       // vm_read8()
#include "cpu/x86_cpu.h" // x86_linear_addr()
#include "cpu/trace.h"

x86_status_t cpu_execute(exec_ctx_t *e)
{
    if (!e || !e->cpu || !e->vm) return X86_ERR;

    uint16_t cs = e->cpu->cs;
    uint16_t ip = e->cpu->ip;

    uint8_t op = 0xFF;
    uint32_t lin = x86_linear_addr(cs, ip);

    if (!vm_read8(e->vm, lin, &op)) {
        fprintf(stderr, "DECODE %04X:%04X [lin=%08X] <fault reading opcode>\n",
                cs, ip, (unsigned)lin);
        return X86_FAULT;
    }

    /* ---- ADD THIS: read a small byte window for trace_pre ---- */
    uint8_t bytes[8] = {0};
    size_t nbytes = 0;
    for (size_t i = 0; i < sizeof(bytes); i++) {
        uint8_t b = 0;
        if (!vm_read8(e->vm, lin + (uint32_t)i, &b)) break;
        bytes[nbytes++] = b;
    }

    /* ---- ADD THIS: trace_pre right after opcode + bytes are known ---- */
    trace_pre(e, op, bytes, nbytes);

    fprintf(stderr, "DECODE %04X:%04X [lin=%08X] op=%02X\n",
            cs, ip, (unsigned)lin, op);

    /* ---- decode ---- */
    x86_fn_t fn = x86_decode_ctx(e);

    /* ---- ADD THIS: trace_decode immediately after decode chooses handler ----
       Keep mnemonic/operands simple for now. You can enrich later. */
    {
        const char *mn = "<unknown>";
        char ops[64] = {0};

        /* Optional: super cheap special-case to prove decode/handler selection.
           This does NOT change behavior; it's just for printing. */
        if ((op & 0xF8) == 0xB8 && nbytes >= 3) { /* MOV r16, imm16 */
            unsigned reg = (unsigned)(op & 0x7);
            uint16_t imm = (uint16_t)(bytes[1] | ((uint16_t)bytes[2] << 8));
            mn = "mov";
            /* reg names: 0 AX,1 CX,2 DX,3 BX,4 SP,5 BP,6 SI,7 DI */
            static const char *r16[8] = {"ax","cx","dx","bx","sp","bp","si","di"};
            snprintf(ops, sizeof(ops), "%s, 0x%04X", r16[reg], imm);
        } else {
            snprintf(ops, sizeof(ops), "op=0x%02X", op);
        }

        trace_decode(e, mn, ops, fn);
    }

    /* ---- execute ---- */
    x86_status_t st = fn(e);

    /* ---- ADD THIS: trace_post right after handler returns ---- */
    trace_post(e, st);

    return st;
}