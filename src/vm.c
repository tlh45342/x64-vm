#include "vm.h"
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
    } else {
        snprintf(v->name, sizeof(v->name), "vm%d", id);
    }

    v->mem = (uint8_t*)calloc(1, ram_bytes);
    if (!v->mem) {
        v->in_use = false;
        return -1;
    }
    v->mem_size = ram_bytes;

    x86_init(&v->cpu, v->mem, v->mem_size);
    v->cpu_inited = true;

    // default start (you can change later)
    v->cpu.cs = 0x0000;
    v->cpu.ip = 0x1000;

    // auto-select created VM
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