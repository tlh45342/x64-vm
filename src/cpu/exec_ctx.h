// exec_ctx.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#include "x64_cpu.h"   // for x86_cpu_t (your existing CPU struct)

// Forward declare VM if you have/plan one (safe even if vm_t isn't defined yet)
typedef struct vm vm_t;

typedef struct exec_ctx {
    x86_cpu_t *cpu;      // always present: registers/flags/segments/IP
    vm_t      *vm;       // optional for now; can be NULL during transition

    // Optional convenience (can be NULL now, filled later when you split VM state)
    uint8_t   *mem;
    size_t     mem_size;

    // Optional trace/flags hooks (future)
    uint32_t   dbg;
    uint32_t   last_phys; // last physical addr touched (handy for debug)
} exec_ctx_t;