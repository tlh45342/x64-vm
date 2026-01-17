// src/cpu/decode.c

/*
 * Copyright © 2025–2026 Thomas L Hamilton
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
    // Use the CPU-based fetch helpers (implemented elsewhere, e.g. cpu_system.c).
    // Declared here to avoid needing header churn.
    extern bool fetch8 (x86_cpu_t *c, uint8_t  *out);
    extern bool fetch16(x86_cpu_t *c, uint16_t *out);

    const uint8_t mod = (modrm >> 6) & 3u;
    const uint8_t rm  = (modrm >> 0) & 7u;

    // mod==3 is register-direct, not a memory EA
    if (mod == 3u) return false;

    // Special case: mod==00 rm==110 => [disp16] (no base regs)
    if (mod == 0u && rm == 6u) {
        uint16_t disp16 = 0;
        if (!fetch16(c, &disp16)) return false;
        *out_ea = disp16;
        return true;
    }

    // Displacement (disp8 sign-extended; disp16 treated as signed here like your original)
    int32_t disp = 0;
    if (mod == 1u) {
        uint8_t d8 = 0;
        if (!fetch8(c, &d8)) return false;
        disp = (int8_t)d8;
    } else if (mod == 2u) {
        uint16_t d16 = 0;
        if (!fetch16(c, &d16)) return false;
        disp = (int16_t)d16;
    }

    uint16_t base = 0;
    switch (rm) {
        case 0: base = (uint16_t)(c->bx + c->si); break; // [BX+SI]
        case 1: base = (uint16_t)(c->bx + c->di); break; // [BX+DI]
        case 2: base = (uint16_t)(c->bp + c->si); break; // [BP+SI]
        case 3: base = (uint16_t)(c->bp + c->di); break; // [BP+DI]
        case 4: base = c->si; break;                     // [SI]
        case 5: base = c->di; break;                     // [DI]
        case 6: base = c->bp; break;                     // [BP] (mod!=0 here)
        case 7: base = c->bx; break;                     // [BX]
    }

    *out_ea = (uint16_t)(base + (uint16_t)disp);
    return true;
}

typedef x86_status_t (*x86_exec_fn_t)(exec_ctx_t *e);

