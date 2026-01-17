// src/cpu/fetch.c

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

#include "cpu/fetch.h"
#include <stdbool.h>
#include <stdint.h>

bool fetch8(x86_cpu_t *c, uint8_t *out) {
    uint32_t a = x86_linear_addr(c->cs, c->ip);
    if (!mem_read8(c, a, out)) return false;
    c->ip++;
    return true;
}

// 8086-style push: SP -= 2; [SS:SP] = val
bool push16(x86_cpu_t *c, uint16_t val) {
    c->sp = (uint16_t)(c->sp - 2);
    uint32_t a = x86_linear_addr(c->ss, c->sp);
    return mem_write16(c, a, val);
}

bool fetch16(x86_cpu_t *c, uint16_t *out) {
    uint32_t a = x86_linear_addr(c->cs, c->ip);
    if (!mem_read16(c, a, out)) return false;
    c->ip = (uint16_t)(c->ip + 2);
    return true;
}

uint32_t x86_linear_addr(uint16_t seg, uint16_t off)
{
    return ((uint32_t)seg << 4) + (uint32_t)off;
}