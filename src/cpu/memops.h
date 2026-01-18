// src/cpu/memops.h

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
#include <stdbool.h>

#include "cpu/exec_ctx.h"

// Instruction stream helpers (CS:IP)
bool x86_fetch8 (exec_ctx_t *e, uint8_t  *out);
bool x86_fetch16(exec_ctx_t *e, uint16_t *out);

bool x86_push16(exec_ctx_t *e, uint16_t val);