// src/cpu/cpu.h

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
#include "exec_ctx.h"

void     x86_init(x86_cpu_t *c, uint8_t *mem, size_t mem_size);
uint32_t x86_linear_addr(uint16_t seg, uint16_t off);

/* register / EA helpers */
uint16_t *x86_reg16(exec_ctx_t *e, unsigned reg);
bool x86_ea16(exec_ctx_t *e, uint8_t modrm, uint16_t *off, uint16_t *seg);

// memory helpers (implemented in decode.c for now)
bool mem_read8 (x86_cpu_t *c, uint32_t addr, uint8_t  *out);
bool mem_read16(x86_cpu_t *c, uint32_t addr, uint16_t *out);
bool mem_write8 (x86_cpu_t *c, uint32_t addr, uint8_t  val);
bool mem_write16(x86_cpu_t *c, uint32_t addr, uint16_t val);

/* instruction fetch */
bool x86_fetch8(exec_ctx_t *e, uint8_t *out);
bool x86_fetch16(exec_ctx_t *e, uint16_t *out);

/* flag helpers */
void x86_set_cf(exec_ctx_t *e, bool v);
void x86_set_zf_sf16(exec_ctx_t *e, uint16_t v);

bool x86_fetch8(exec_ctx_t *e, uint8_t *out);
bool x86_fetch16(exec_ctx_t *e, uint16_t *out);

uint16_t *x86_reg16(exec_ctx_t *e, unsigned reg);

bool x86_read16(exec_ctx_t *e, uint16_t seg, uint16_t off, uint16_t *out);
bool x86_write16(exec_ctx_t *e, uint16_t seg, uint16_t off, uint16_t val);

void x86_set_cf(exec_ctx_t *e, bool v);
void x86_set_zf_sf16(exec_ctx_t *e, uint16_t r);