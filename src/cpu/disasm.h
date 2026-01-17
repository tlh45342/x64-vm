// src/cpu/disasm.h

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    size_t len;        // bytes consumed
    char   text[64];   // disassembly
    bool   ok;         // decoded something known
} x86_disasm_t;

x86_disasm_t x86_disasm_one_16(const uint8_t *mem, size_t memsz, uint32_t lin);
