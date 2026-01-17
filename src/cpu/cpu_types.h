// src/cpu/cpu_type.h

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

typedef enum {
    X86_OK = 0,
    X86_HALT = 1,
   
     // Negative = error/fault class
    X86_ERR     = -1,
    X86_ILLEGAL = -2,   // unknown/unsupported opcode
    X86_FAULT   = -3    // bad memory fetch/seg fault/etc
} x86_status_t;

/* FLAGS bits (16-bit real mode) */
enum {
    X86_FL_CF = 1u << 0,
    X86_FL_PF = 1u << 2,
    X86_FL_AF = 1u << 4,
    X86_FL_ZF = 1u << 6,
    X86_FL_SF = 1u << 7,
    X86_FL_TF = 1u << 8,
    X86_FL_IF = 1u << 9,
    X86_FL_DF = 1u << 10,
    X86_FL_OF = 1u << 11
};
