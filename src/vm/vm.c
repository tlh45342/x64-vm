// src/vm/vm.c

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

#include "vm/vm.h"
#include "cpu/disasm.h"
#include "util/log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void vmman_init(VMManager *m) {
    memset(m, 0, sizeof(*m));
    m->current = -1;
    for (int i = 0; i < VM_MAX; i++) {
        m->vms[i].id = i;
        m->vms[i].in_use = false;
    }
}

static int find_free_slot(VMManager *m) {
    for (int i = 0; i < VM_MAX; i++) {
        if (!m->vms[i].in_use) return i;
    }
    return -1;
}

int vm_create_default(VMManager *m, size_t ram_bytes, const char *name) {
    int id = find_free_slot(m);
    if (id < 0) return -1;

    VM *v = &m->vms[id];
    memset(v, 0, sizeof(*v));
    v->id = id;
    v->in_use = true;

    if (name && *name) {
        strncpy(v->name, name, sizeof(v->name) - 1);
        v->name[sizeof(v->name) - 1] = '\0';
    } else {
        snprintf(v->name, sizeof(v->name), "vm%d", id);
    }

    /* trace/log defaults */
    v->trace.enabled = false;
    v->trace.flags   = 0;
    v->log           = NULL;

    v->mem = (uint8_t*)calloc(1, ram_bytes);
    if (!v->mem) {
        v->in_use = false;
        return -1;
    }
    v->mem_size = ram_bytes;

    x86_init(&v->cpu, v->mem, v->mem_size);
    v->cpu_inited = true;

    /* default start (you can change later) */
    v->cpu.cs = 0x0000;
    v->cpu.ip = 0x1000;

    /* auto-select created VM */
    m->current = id;
    return id;
}

bool vm_destroy(VMManager *m, int id) {
    VM *v = vm_get(m, id);
    if (!v) return false;

    free(v->mem);
    v->mem = NULL;
    v->mem_size = 0;
    v->cpu_inited = false;
    v->in_use = false;
    v->name[0] = '\0';

    if (m->current == id) m->current = -1;
    return true;
}

bool vm_use(VMManager *m, int id) {
    VM *v = vm_get(m, id);
    if (!v) return false;
    m->current = id;
    return true;
}

VM *vm_get(VMManager *m, int id) {
    if (!m) return NULL;
    if (id < 0 || id >= VM_MAX) return NULL;
    if (!m->vms[id].in_use) return NULL;
    return &m->vms[id];
}

VM *vm_current(VMManager *m) {
    return vm_get(m, m->current);
}

void vm_list(VMManager *m) {
    printf("VMs:\n");
    for (int i = 0; i < VM_MAX; i++) {
        VM *v = &m->vms[i];
        if (!v->in_use) continue;
        printf("  %c id=%d name=%s ram=%zu\n",
               (m->current == i) ? '*' : ' ',
               i, v->name, v->mem_size);
    }
}

x86_status_t vm_step(VM *vm) {
    if (!vm) return X86_HALT; /* or whatever "bad" status you prefer */
    x86_cpu_t *c = &vm->cpu;

    /* ---- TRACE PRE ---- */
    if (vm->trace.enabled && vm->log) {
        uint32_t lin = ((uint32_t)c->cs << 4) + c->ip;

        char bytes[3 * 16 + 1];
        size_t n = 0;
        for (unsigned i = 0; i < 16 && (size_t)(lin + i) < vm->mem_size; i++) {
            n += (size_t)snprintf(bytes + n, sizeof(bytes) - n,
                                  "%02X ", vm->mem[lin + i]);
            if (n >= sizeof(bytes)) break;
        }
        if (n) bytes[(n - 1 < sizeof(bytes)) ? (n - 1) : (sizeof(bytes) - 1)] = 0;

        x86_disasm_t d = x86_disasm_one_16(vm->mem, vm->mem_size, lin);

        log_printf(vm->log, LOG_TRACE, "dis",
                   "%04X:%04X  %-47s  %s\n",
                   c->cs, c->ip, bytes, d.text);
    }

    /* ---- EXECUTE ---- */
    x86_status_t st = x86_step(c);

    /* ---- TRACE POST ---- */
    if (vm->trace.enabled && vm->log) {
        log_printf(vm->log, LOG_TRACE, "cpu",
                   "AX=%04X BX=%04X CX=%04X DX=%04X "
                   "CS:IP=%04X:%04X FLAGS=%04X\n",
                   c->ax, c->bx, c->cx, c->dx,
                   c->cs, c->ip, c->flags);
    }

    return st;
}
