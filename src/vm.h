#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cpu/cpu.h"   // x86_cpu_t

#ifndef VM_MAX
#define VM_MAX 8
#endif

typedef struct VM {
    int id;
    bool in_use;

    char name[32];

    uint8_t *mem;
    size_t   mem_size;

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