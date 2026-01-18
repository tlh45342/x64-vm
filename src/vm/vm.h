// src/vm/vm.h

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
#include <stddef.h>
#include <stdbool.h>

#include "cpu/x86_cpu.h"   // x86_cpu_t, x86_status_t

#ifndef VM_MAX
#define VM_MAX 8
#endif

/* forward declare logger type from util/log.h */
typedef struct logger logger_t;

typedef struct trace_cfg {
    bool enabled;      /* runtime enable */
    unsigned flags;    /* future: TRACE_* bitflags */
} trace_t;

typedef struct VM {
    int id;
    bool in_use;
    char name[32];

    /* tracing/logging (optional) */
    trace_t   trace;
    logger_t *log;

    /* RAM backing */
    uint8_t *mem;
    size_t   mem_size;

    /* CPU state */
    x86_cpu_t cpu;
    bool cpu_inited;
} VM;

typedef struct VMManager {
    VM vms[VM_MAX];
    int current; // -1 if none
} VMManager;

void  vmman_init(VMManager *m);
int   vm_create_default(VMManager *m, size_t ram_bytes, const char *name);
bool  vm_destroy(VMManager *m, int id);
bool  vm_use(VMManager *m, int id);
VM   *vm_get(VMManager *m, int id);
VM   *vm_current(VMManager *m);
void  vm_list(VMManager *m);
bool  vm_read8 (VM *vm, uint32_t addr, uint8_t *out);
bool  vm_read16(VM *vm, uint32_t addr, uint16_t *out);
bool  vm_write8 (VM *vm, uint32_t addr, uint8_t val);
bool  vm_write16(VM *vm, uint32_t addr, uint16_t val);

/* Execute one instruction on the given VM */
x86_status_t vm_step(VM *vm);
