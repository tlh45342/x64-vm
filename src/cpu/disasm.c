#include "disasm.h"
#include <stdio.h>
#include <string.h>

static const char *reg16_name(unsigned r) {
    static const char *r16[8] = {"ax","cx","dx","bx","sp","bp","si","di"};
    return (r < 8) ? r16[r] : "??";
}

x86_disasm_t x86_disasm_one_16(const uint8_t *mem, size_t memsz, uint32_t lin) {
    x86_disasm_t d;
    memset(&d, 0, sizeof(d));
    d.ok = false;
    d.len = 1;
    snprintf(d.text, sizeof(d.text), "db ?");

    if (!mem || lin >= memsz) {
        snprintf(d.text, sizeof(d.text), "<oob>");
        return d;
    }

    uint8_t op = mem[lin];

    // HLT
    if (op == 0xF4) {
        d.ok = true;
        d.len = 1;
        snprintf(d.text, sizeof(d.text), "hlt");
        return d;
    }

    // NOP (handy)
    if (op == 0x90) {
        d.ok = true;
        d.len = 1;
        snprintf(d.text, sizeof(d.text), "nop");
        return d;
    }

    // MOV r16, imm16  (B8..BF)
    if (op >= 0xB8 && op <= 0xBF) {
        if (lin + 2 < memsz) {
            uint16_t imm = (uint16_t)mem[lin + 1] | ((uint16_t)mem[lin + 2] << 8);
            unsigned reg = (unsigned)(op - 0xB8);
            d.ok = true;
            d.len = 3;
            snprintf(d.text, sizeof(d.text), "mov %s, 0x%04X", reg16_name(reg), imm);
            return d;
        } else {
            snprintf(d.text, sizeof(d.text), "mov r16, <trunc>");
            d.len = 1;
            return d;
        }
    }

    // Unknown
    snprintf(d.text, sizeof(d.text), "db 0x%02X", op);
    d.len = 1;
    return d;
}