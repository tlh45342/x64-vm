// src/cpu/interrupt.c

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
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include "cpu/interrupt.h"
#include "cpu/memops.h"
#include "vm/vm.h"
#include "cpu/x86_cpu.h"

// IVT entry n at physical 0x0000: (offset @ 4n, segment @ 4n+2)
bool ivt_get_vector(exec_ctx_t *e, uint8_t n, uint16_t *out_ip, uint16_t *out_cs)
{
    if (!e || !e->vm) return false;

    uint32_t base = (uint32_t)n * 4u;
    uint16_t ip, cs;

    if (!vm_read16(e->vm, base + 0, &ip)) return false;
    if (!vm_read16(e->vm, base + 2, &cs)) return false;

    *out_ip = ip;
    *out_cs = cs;
    return true;
}

x86_status_t handle_int_cd(exec_ctx_t *e)
{
    x86_cpu_t *c = e->cpu;
    if (!e || !c || !e->vm) return X86_ERR;

    uint8_t n = 0;
    if (!x86_fetch8(e, &n)) return X86_ERR;

    // Push FLAGS, CS, IP (IP already points to next instruction after imm8)
    if (!x86_push16(e, c->flags)) return X86_ERR;
    if (!x86_push16(e, c->cs))    return X86_ERR;
    if (!x86_push16(e, c->ip))    return X86_ERR;

    // Clear IF and TF (bits 9 and 8)
    c->flags = (uint16_t)(c->flags & ~(uint16_t)0x0300u);

    uint16_t new_ip = 0, new_cs = 0;
    if (!ivt_get_vector(e, n, &new_ip, &new_cs)) return X86_ERR;

    c->cs = new_cs;
    c->ip = new_ip;
    return X86_OK;
}