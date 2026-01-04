// logic.c

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "exec_ctx.h"
#include "x64_cpu.h"   // for x86_cpu_t (your existing CPU struct)
#include "x86_ops.h"

x86_status_t handle_grp1_83(exec_ctx_t *e) {
    uint8_t modrm;
    if (!x86_fetch8(e, &modrm)) return X86_ERR;

    uint8_t sub = (modrm >> 3) & 7u;

    switch (sub) {
        case 5u: return handle_sub_83_5(e, modrm); // SUB r/m16, imm8
        case 7u: return handle_cmp_83_7(e, modrm);
        case 0u: return handle_add_83_0(e, modrm);
        case 2u: return handle_adc_83_2(e, modrm);
        default:
            fprintf(stderr, "grp1 83 subop %u not implemented\n", sub);
            return X86_ERR;
    }
}

x86_status_t handle_sub_83_5(exec_ctx_t *e, uint8_t modrm) {
    // compute EA early if memory
    uint8_t mod = (modrm >> 6) & 3u;
    uint8_t rm  = (modrm >> 0) & 7u;

    uint16_t dst = 0;
    uint16_t seg = 0, off = 0;

    if (mod == 3u) {
        dst = *x86_reg16(e, rm);
    } else {
        if (!x86_ea16(e, modrm, &off, &seg)) return X86_ERR;
        if (!x86_read16(e, seg, off, &dst)) return X86_ERR;
    }

    uint8_t imm8u;
    if (!x86_fetch8(e, &imm8u)) return X86_ERR;
    uint16_t imm16 = (uint16_t)(int16_t)(int8_t)imm8u;

    uint32_t wide = (uint32_t)dst - (uint32_t)imm16;
    uint16_t res  = (uint16_t)wide;

    // CF = borrow (dst < imm)
    x86_set_cf(e, dst < imm16);
    x86_set_zf_sf16(e, res);
    // OF later

    if (mod == 3u) {
        *x86_reg16(e, rm) = res;
    } else {
        if (!x86_write16(e, seg, off, res)) return X86_ERR;
    }
    return X86_OK;
}

static void set_zf_sf16(x86_cpu_t *c, uint16_t r) {
    if (r == 0) c->flags |= (uint16_t)X86_FL_ZF;
    else        c->flags &= (uint16_t)~X86_FL_ZF;

    if (r & 0x8000u) c->flags |= (uint16_t)X86_FL_SF;
    else             c->flags &= (uint16_t)~X86_FL_SF;
}

static void update_flags_sub(x86_cpu_t *c, uint16_t old_val, uint16_t sub_val, uint16_t result) {
    // Zero flag
    if (result == 0) c->flags |= (uint16_t)X86_FL_ZF;
    else             c->flags &= (uint16_t)~X86_FL_ZF;

    // Sign flag
    if (result & 0x8000u) c->flags |= (uint16_t)X86_FL_SF;
    else                  c->flags &= (uint16_t)~X86_FL_SF;

    // Carry / borrow
    if (old_val < sub_val) c->flags |= (uint16_t)X86_FL_CF;
    else                   c->flags &= (uint16_t)~X86_FL_CF;

    // Overflow (signed)
    if (((old_val ^ sub_val) & (old_val ^ result) & 0x8000u) != 0) c->flags |= (uint16_t)X86_FL_OF;
    else                                                           c->flags &= (uint16_t)~X86_FL_OF;
}

x86_status_t op_mov_r16_imm16(exec_ctx_t *e) {
    // uses decoded fields: e->reg, e->imm16
}


