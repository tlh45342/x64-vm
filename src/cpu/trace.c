// src/cpu/trace.c

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
#include "cpu/trace.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "vm/vm.h"       // VM (for vm->logger field; adjust if needed)
#include "util/log.h"    // logger_t, log_printf, logger_enabled

/* ---- 10-second fuzzy knobs ---- */

static int trace_debug_enabled(const exec_ctx_t *e) {
    (void)e;
    /* TODO: replace with real debug flag check */
    return 1; /* ON for now while debugging */
}

/*
 * IMPORTANT:
 * I have to assume where the logger lives.
 * The compile error proves it's NOT VM*.
 *
 * Most likely you have either:
 *   - vm->lg
 *   - vm->logger
 *   - vm->log
 *
 * Pick the right one. I default to vm->lg below.
 */
static logger_t *trace_logger(exec_ctx_t *e) {
    if (!e || !e->vm) return NULL;

    /* VM has member 'log' */
    return e->vm->log;          /* vm->log is logger_t* */
    /* If this doesn't compile (type mismatch), use: return &e->vm->log; */
}

static int trace_logging_enabled(exec_ctx_t *e) {
    logger_t *lg = trace_logger(e);
    /* logger_enabled() also checks level filtering */
    return (lg != NULL) && logger_enabled(lg, LOG_TRACE);
}

/* ---- sink: stderr + (optional) log ---- */

static void trace_printf(exec_ctx_t *e, const char *fmt, ...) {
    if (!trace_debug_enabled(e)) return;

    va_list ap;
    va_start(ap, fmt);

    /* Always print to stderr (your request) */
    vfprintf(stderr, fmt, ap);

    /* Also dump to log if enabled */
    if (trace_logging_enabled(e)) {
        char buf[512];

        va_list ap2;
        va_copy(ap2, ap);
        vsnprintf(buf, sizeof(buf), fmt, ap2);
        va_end(ap2);

        logger_t *lg = trace_logger(e);
        log_printf(lg, LOG_TRACE, "cpu", "%s", buf);
    }

    va_end(ap);
}

/* ---- helpers ---- */

static void dump_bytes(char *out, size_t outsz, const uint8_t *bytes, size_t n) {
    if (!out || outsz == 0) return;
    out[0] = 0;

    size_t pos = 0;
    for (size_t i = 0; i < n; i++) {
        int wrote = snprintf(out + pos, (pos < outsz ? outsz - pos : 0),
                             "%02X%s", bytes[i], (i + 1 < n ? " " : ""));
        if (wrote <= 0) break;
        pos += (size_t)wrote;
        if (pos + 1 >= outsz) break;
    }
}

static const char *x86_status_name(x86_status_t st) {
    switch (st) {
        case X86_OK:    return "OK";
        case X86_HALT:  return "HALT";
        case X86_FAULT: return "FAULT";
        case X86_ERR:   return "ERR";
        default:        return "STATUS";
    }
}

/* ---- public API ---- */

void trace_pre(exec_ctx_t *e, uint8_t op, const uint8_t *bytes, size_t nbytes) {
    if (!trace_debug_enabled(e) || !e || !e->cpu) return;

    const x86_cpu_t *c = e->cpu;

    char bbuf[128];
    dump_bytes(bbuf, sizeof(bbuf), bytes, nbytes);

    trace_printf(e,
        "TRACE PRE  %04X:%04X  op=%02X  bytes=[%s]\n"
        "          AX=%04X BX=%04X CX=%04X DX=%04X  SI=%04X DI=%04X BP=%04X SP=%04X\n"
        "          CS=%04X IP=%04X DS=%04X ES=%04X SS=%04X  FLAGS=%04X\n",
        c->cs, c->ip, op, bbuf,
        c->ax, c->bx, c->cx, c->dx, c->si, c->di, c->bp, c->sp,
        c->cs, c->ip, c->ds, c->es, c->ss, c->flags
    );
}

void trace_decode(exec_ctx_t *e, const char *mnemonic, const char *operands, x86_fn_t fn) {
    if (!trace_debug_enabled(e) || !e || !e->cpu) return;

    const x86_cpu_t *c = e->cpu;

    trace_printf(e,
        "TRACE DEC  %04X:%04X  %s %s  fn=%p\n",
        c->cs, c->ip,
        (mnemonic ? mnemonic : "<unknown>"),
        (operands ? operands : ""),
        (void*)fn
    );
}

void trace_post(exec_ctx_t *e, x86_status_t st) {
    if (!trace_debug_enabled(e) || !e || !e->cpu) return;

    const x86_cpu_t *c = e->cpu;

    trace_printf(e,
        "TRACE POST %04X:%04X  status=%s(%d)\n"
        "          AX=%04X BX=%04X CX=%04X DX=%04X  SI=%04X DI=%04X BP=%04X SP=%04X\n"
        "          CS=%04X IP=%04X DS=%04X ES=%04X SS=%04X  FLAGS=%04X\n",
        c->cs, c->ip,
        x86_status_name(st), (int)st,
        c->ax, c->bx, c->cx, c->dx, c->si, c->di, c->bp, c->sp,
        c->cs, c->ip, c->ds, c->es, c->ss, c->flags
    );
}