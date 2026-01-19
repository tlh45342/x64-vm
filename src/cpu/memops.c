// src/cpu/memops.c

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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "cpu/memops.h"
#include "vm/vm.h"
#include "cpu/x86_cpu.h"

bool x86_fetch8(exec_ctx_t *e, uint8_t *out)
{
    x86_cpu_t *c = e->cpu;
    uint32_t a = x86_linear_addr(c->cs, c->ip);

	fprintf(stderr, "FETCH8 %04X:%04X -> %02X\n", e->cpu->cs, e->cpu->ip, *out);

    if (!e->vm) return false;
    if (!vm_read8(e->vm, a, out)) return false;

    c->ip++;
    return true;
}

bool x86_fetch16(exec_ctx_t *e, uint16_t *out)
{
    x86_cpu_t *c = e->cpu;
    uint32_t a = x86_linear_addr(c->cs, c->ip);

    if (!e->vm) return false;
    if (!vm_read16(e->vm, a, out)) return false;

    c->ip = (uint16_t)(c->ip + 2);
    return true;
}

// 8086-style push: SP -= 2; [SS:SP] = val
bool x86_push16(exec_ctx_t *e, uint16_t val)
{
    x86_cpu_t *c = e->cpu;

    if (!e->vm) return false;

    c->sp = (uint16_t)(c->sp - 2);
    uint32_t a = x86_linear_addr(c->ss, c->sp);

    return vm_write16(e->vm, a, val);
}

