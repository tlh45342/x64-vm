// src/cpu/trace.h

/*
 * Copyright 2026 Thomas L Hamilton
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

#include <stdint.h>
#include <stddef.h>

#include "cpu_types.h"
#include "exec_ctx.h"
#include "cpu/table.h"

void trace_pre(exec_ctx_t *e, uint8_t op, const uint8_t *bytes, size_t nbytes);
void trace_decode(exec_ctx_t *e, const char *mnemonic, const char *operands, x86_fn_t fn);
void trace_post(exec_ctx_t *e, x86_status_t st);