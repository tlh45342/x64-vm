// src/cpu/x86_cpu.h

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

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cpu_types.h"

// Forward declare to avoid include cycles.
// (exec_ctx_t is defined in exec_ctx.h; we only need the name here.)
typedef struct exec_ctx exec_ctx_t;

typedef struct x86_cpu {
    // 16-bit real-mode registers (current scope)
    uint16_t ax, bx, cx, dx;
    uint16_t sp, bp, si, di;

    uint16_t cs, ds, es, ss;
    uint16_t ip;
    uint16_t flags;

    bool halted;
    bool rep_prefix;        // set when 0xF3 seen, consumed by next string op

    // Transitional: memory currently lives here, but you're moving it behind VM.
    uint8_t *mem;
    size_t   mem_size;
} x86_cpu_t;

// init/reset
void x86_init(x86_cpu_t *c, uint8_t *mem, size_t mem_size);

// address helper
uint32_t x86_linear_addr(uint16_t seg, uint16_t off);

// register helper(s)
uint16_t *x86_reg16(exec_ctx_t *e, unsigned reg);

// flag helper(s)
void x86_set_cf(exec_ctx_t *e, bool v);
void x86_set_zf_sf16(exec_ctx_t *e, uint16_t r);

x86_status_t x86_step(exec_ctx_t *e);
