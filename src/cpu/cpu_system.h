// src/cpu_system.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "x64_cpu.h"   // x86_cpu_t, x86_status_t

// Handle INT imm8 (opcode 0xCD). Assumes opcode already consumed.
x86_status_t handle_int_cd(x86_cpu_t *c);