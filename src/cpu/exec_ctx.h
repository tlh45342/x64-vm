// src/cpu/exec_ctx.h

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

#pragma once
#include <stdint.h>

typedef struct VM VM;      // forward declare the real VM type
typedef struct x86_cpu x86_cpu_t;  // forward declare CPU

typedef struct exec_ctx {
    x86_cpu_t *cpu;     // always present
    VM        *vm;      // VM owns memory/devices; may be NULL during transition

    // Optional trace/flags hooks (future)
    uint32_t   dbg;
    uint32_t   last_phys; // last physical addr touched (handy for debug)
} exec_ctx_t;