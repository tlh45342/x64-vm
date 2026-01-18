//  src/cpu/logic.h

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

#pragma once

#include "cpu/exec_ctx.h"
#include "cpu/cpu_types.h"

x86_status_t handle_grp1_83(exec_ctx_t *e);
x86_status_t op_mov_r16_imm16(exec_ctx_t *e);