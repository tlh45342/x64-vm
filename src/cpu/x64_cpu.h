// src/cpu/x64_cpu.h

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

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cpu_types.h"

typedef struct x86_cpu {
    // 16-bit real-mode registers for now
    uint16_t ax, bx, cx, dx;
    uint16_t sp, bp, si, di;

    uint16_t cs, ds, es, ss;
    uint16_t ip;
    uint16_t flags; // not used yet

    bool halted;
	bool rep_prefix;   // set when 0xF3 seen, consumed by next string instruction
    uint8_t *mem;
    size_t mem_size;
} x86_cpu_t;

void x86_init(x86_cpu_t *c, uint8_t *mem, size_t mem_size);
uint32_t x86_linear_addr(uint16_t seg, uint16_t off);
x86_status_t x86_step(x86_cpu_t *c);