// src/cpu/execute.c

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

#include "cpu_types.h"
#include "exec_ctx.h"
#include "execute.h"
#include "cpu.h"
#include "cpu/disasm.h"
#include "util/log.h"

// Declare the decoder entrypoint (so we don't have to touch headers yet).
typedef x86_status_t (*x86_exec_fn_t)(exec_ctx_t *e);
extern x86_exec_fn_t x86_decode_ctx(exec_ctx_t *e);

x86_status_t cpu_execute(exec_ctx_t *e) {
    if (!e) return X86_FAULT;

    // 1) decode -> returns handler function pointer (table dispatch)
    x86_exec_fn_t fn = x86_decode_ctx(e);
    if (!fn) return X86_ILLEGAL;   // unknown/unsupported instruction

    // 2) execute handler
    return fn(e);
}