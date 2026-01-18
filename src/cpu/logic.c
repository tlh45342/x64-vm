// src/cpu/logic.c

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

#include <stdint.h>
#include <stdbool.h>

#include "cpu/logic.h"
#include "cpu/x86_cpu.h"
#include "cpu/memops.h"

/* ============================================================
 * Local flag helpers
 * ============================================================ */

static inline void set_flag_u16(x86_cpu_t *c, uint16_t mask, bool v)
{
    if (v) c->flags |= mask;
    else   c->flags &= (uint16_t)~mask;
}

static inline bool get_cf(const x86_cpu_t *c)
{
    return (c->flags & (uint16_t)X86_FL_CF) != 0;
}

static void update_flags_add16(exec_ctx_t *e, uint16_t dst, uint16_t src, uint16_t res)
{
    x86_cpu_t *c = e->cpu;

    /* CF: unsigned carry out */
    set_flag_u16(c, (uint16_t)X86_FL_CF, res < dst);

    /* OF: signed overflow */
    set_flag_u16(c, (uint16_t)X86_FL_OF,
                 (((uint16_t)~(dst ^ src) & (dst ^ res) & 0x8000u) != 0));

    /* ZF/SF */
    x86_set_zf_sf16(e, res);
}

static void update_flags_sub16(exec_ctx_t *e, uint16_t dst, uint16_t src, uint16_t res)
{
    x86_cpu_t *c = e->cpu;

    /* CF: borrow -> dst < src */
    set_flag_u16(c, (uint16_t)X86_FL_CF, dst < src);

    /* OF: signed overflow for subtraction */
    set_flag_u16(c, (uint16_t)X86_FL_OF,
                 (((dst ^ src) & (dst ^ res) & 0x8000u) != 0));

    /* ZF/SF */
    x86_set_zf_sf16(e, res);
}

/* ============================================================
 * Group 1 (0x80/0x81/0x83) handlers – we start with 0x83
 * For now we only support modrm.mod == 3 (register-direct).
 * ============================================================ */

static x86_status_t handle_add_83_0(exec_ctx_t *e, uint8_t modrm);
static x86_status_t handle_adc_83_2(exec_ctx_t *e, uint8_t modrm);
static x86_status_t handle_sub_83_5(exec_ctx_t *e, uint8_t modrm);
static x86_status_t handle_cmp_83_7(exec_ctx_t *e, uint8_t modrm);

/*
 * Dispatch for opcode 0x83 /r
 * reg field selects operation:
 *   /0 ADD r/m16, imm8
 *   /2 ADC r/m16, imm8
 *   /5 SUB r/m16, imm8
 *   /7 CMP r/m16, imm8
 */
x86_status_t handle_grp1_83(exec_ctx_t *e)
{
    uint8_t modrm = 0;
    if (!x86_fetch8(e, &modrm)) return X86_ERR;

    uint8_t op = (uint8_t)((modrm >> 3) & 7u);

    switch (op) {
        case 0u: return handle_add_83_0(e, modrm);
        case 2u: return handle_adc_83_2(e, modrm);
        case 5u: return handle_sub_83_5(e, modrm);
        case 7u: return handle_cmp_83_7(e, modrm);
        default:
            return X86_ERR; /* unimplemented /1 OR /3 OR /4 OR /6 */
    }
}

static bool modrm_is_reg(uint8_t modrm)
{
    return ((modrm >> 6) & 3u) == 3u;
}

static uint16_t signext_imm8_to_u16(uint8_t imm8)
{
    return (uint16_t)(int16_t)(int8_t)imm8;
}

/* 0x83 /0 : ADD r/m16, imm8 */
static x86_status_t handle_add_83_0(exec_ctx_t *e, uint8_t modrm)
{
    if (!modrm_is_reg(modrm)) return X86_ERR; /* EA path not wired yet */

    uint8_t imm8 = 0;
    if (!x86_fetch8(e, &imm8)) return X86_ERR;

    uint16_t src = signext_imm8_to_u16(imm8);
    unsigned rm = (unsigned)(modrm & 7u);

    uint16_t *dstp = x86_reg16(e, rm);
    uint16_t dst = *dstp;
    uint16_t res = (uint16_t)(dst + src);

    update_flags_add16(e, dst, src, res);

    *dstp = res;
    return X86_OK;
}

/* 0x83 /2 : ADC r/m16, imm8 */
static x86_status_t handle_adc_83_2(exec_ctx_t *e, uint8_t modrm)
{
    if (!modrm_is_reg(modrm)) return X86_ERR; /* EA path not wired yet */

    uint8_t imm8 = 0;
    if (!x86_fetch8(e, &imm8)) return X86_ERR;

    uint16_t src = signext_imm8_to_u16(imm8);
    unsigned rm = (unsigned)(modrm & 7u);

    uint16_t *dstp = x86_reg16(e, rm);
    uint16_t dst = *dstp;

    uint16_t carry = get_cf(e->cpu) ? 1u : 0u;
    uint16_t res = (uint16_t)(dst + src + carry);

    /* CF for adc is more subtle; easiest is 32-bit sum */
    uint32_t sum32 = (uint32_t)dst + (uint32_t)src + (uint32_t)carry;
    set_flag_u16(e->cpu, (uint16_t)X86_FL_CF, (sum32 & 0x10000u) != 0);

    /* OF for adc uses src+carry as effective src for signed overflow test */
    {
        uint16_t eff_src = (uint16_t)(src + carry);
        set_flag_u16(e->cpu, (uint16_t)X86_FL_OF,
                     (((uint16_t)~(dst ^ eff_src) & (dst ^ res) & 0x8000u) != 0));
    }

    x86_set_zf_sf16(e, res);

    *dstp = res;
    return X86_OK;
}

/* 0x83 /5 : SUB r/m16, imm8 */
static x86_status_t handle_sub_83_5(exec_ctx_t *e, uint8_t modrm)
{
    if (!modrm_is_reg(modrm)) return X86_ERR; /* EA path not wired yet */

    uint8_t imm8 = 0;
    if (!x86_fetch8(e, &imm8)) return X86_ERR;

    uint16_t src = signext_imm8_to_u16(imm8);
    unsigned rm = (unsigned)(modrm & 7u);

    uint16_t *dstp = x86_reg16(e, rm);
    uint16_t dst = *dstp;
    uint16_t res = (uint16_t)(dst - src);

    update_flags_sub16(e, dst, src, res);

    *dstp = res;
    return X86_OK;
}

/* 0x83 /7 : CMP r/m16, imm8 (like SUB, but no writeback) */
static x86_status_t handle_cmp_83_7(exec_ctx_t *e, uint8_t modrm)
{
    if (!modrm_is_reg(modrm)) return X86_ERR; /* EA path not wired yet */

    uint8_t imm8 = 0;
    if (!x86_fetch8(e, &imm8)) return X86_ERR;

    uint16_t src = signext_imm8_to_u16(imm8);
    unsigned rm = (unsigned)(modrm & 7u);

    uint16_t dst = *x86_reg16(e, rm);
    uint16_t res = (uint16_t)(dst - src);

    update_flags_sub16(e, dst, src, res);

    return X86_OK;
}

/* ============================================================
 * Placeholder op(s) — keep build clean while you wire decode/exec
 * ============================================================ */

x86_status_t op_mov_r16_imm16(exec_ctx_t *e)
{
    (void)e;
    return X86_ERR; /* TODO: implement once decode provides reg + imm16 */
}