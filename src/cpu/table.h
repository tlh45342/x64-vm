// src/cpu/table.h

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

#include "exec_ctx.h"
#include "cpu_types.h"   // for x86_status_t

// Instruction handler function pointer type
typedef x86_status_t (*x86_fn_t)(exec_ctx_t *e);

// Decode one instruction at CS:IP and return the handler to execute it.
// (Decoder consumes the opcode byte; handlers consume any ModRM/imm/etc.)
x86_fn_t x86_decode_ctx(exec_ctx_t *e);