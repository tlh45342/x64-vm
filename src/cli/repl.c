// src/repl.c

/*
 * Copyright © 2025-2026 Thomas L Hamilton
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include "repl.h"
#include "version.h"

// IMPORTANT: after moving CPU into src/cpu, include it via this path.
// Ensure Makefile has: CFLAGS += -Isrc -Isrc/cpu
#include "cpu/x64_cpu.h"   // x86_cpu_t, x86_init, x86_step, x86_status_t, X86_OK, etc.

/* -----------------------------------------------------------------------------
   VM registry (minimal)
----------------------------------------------------------------------------- */

#ifndef VM_MAX
#define VM_MAX 8
#endif

typedef struct VM {
    bool in_use;
    int  id;
    char name[32];

    uint8_t *mem;
    size_t   mem_size;

    x86_cpu_t cpu;
    bool cpu_inited;
} VM;

typedef struct VMManager {
    VM vms[VM_MAX];
    int current; // -1 if none
} VMManager;

/* -----------------------------------------------------------------------------
   REPL state
----------------------------------------------------------------------------- */

struct repl_state {
    VMManager vmman;

    char img_path[512];          // kept for future (boot/disk)
    uint32_t default_max_steps;

    bool trace;
    FILE *log;
};


// forward decalares
static int cmd_examine(VM *vm, uint16_t seg, uint16_t off, size_t count);


/* -----------------------------------------------------------------------------
   small utils
----------------------------------------------------------------------------- */

static void trim_right(char *s) {
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1])))
        s[--n] = 0;
}

static char *trim_left(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        p++;
    return p;
}

static bool parse_u16_hex(const char *s, uint16_t *out) {
    s = skip_ws(s);
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 16);   // FORCE HEX
    if (end == s) return false;

    end = (char*)skip_ws(end);
    if (*end != '\0') return false;

    if (v > 0xFFFFul) return false;
    *out = (uint16_t)v;
    return true;
}

static void log_printf(repl_state_t *s, const char *fmt, ...) {
    if (!s->log) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(s->log, fmt, ap);
    va_end(ap);
    fputc('\n', s->log);
    fflush(s->log);
}

static bool parse_u16(const char *s, uint16_t *out) {
    if (!s || !*s) return false;
    s = skip_ws(s);

    char *end = NULL;
    unsigned long v = strtoul(s, &end, 0);   // base 0: 123, 0x123, 077, etc.
    if (end == s) return false;

    end = (char*)skip_ws(end);
    if (*end != '\0') return false;

    if (v > 0xFFFFul) return false;
    *out = (uint16_t)v;
    return true;
}

static bool parse_seg_off(const char *s, uint16_t *seg, uint16_t *off) {
    const char *colon = strchr(s, ':');
    if (!colon) return false;

    char a[32], b[32];
    size_t n1 = (size_t)(colon - s);
    size_t n2 = strlen(colon + 1);
    if (n1 == 0 || n2 == 0) return false;
    if (n1 >= sizeof(a) || n2 >= sizeof(b)) return false;

    memcpy(a, s, n1); a[n1] = 0;
    memcpy(b, colon + 1, n2); b[n2] = 0;

    return parse_u16_hex(a, seg) && parse_u16_hex(b, off);
}

static size_t parse_memsize(const char *s, int *ok) {
    // supports: 128M, 64K, 1048576
    *ok = 0;
    if (!s || !*s) return 0;

    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (end == s) return 0;

    size_t mul = 1;
    if (*end) {
        if (end[1] != 0) return 0; // only single suffix char
        switch (*end) {
            case 'k': case 'K': mul = 1024u; break;
            case 'm': case 'M': mul = 1024u * 1024u; break;
            default: return 0;
        }
    }
    *ok = 1;
    return (size_t)(v * (unsigned long long)mul);
}

/* -----------------------------------------------------------------------------
   VM manager impl
----------------------------------------------------------------------------- */

static void vmman_init(VMManager *m) {
    memset(m, 0, sizeof(*m));
    m->current = -1;
    for (int i = 0; i < VM_MAX; i++) {
        m->vms[i].id = i;
        m->vms[i].in_use = false;
    }
}

static VM *vm_get(VMManager *m, int id) {
    if (id < 0 || id >= VM_MAX) return NULL;
    if (!m->vms[id].in_use) return NULL;
    return &m->vms[id];
}

static VM *vm_current(VMManager *m) {
    return vm_get(m, m->current);
}

static int vm_create_default(VMManager *m, size_t ram_bytes, const char *name) {
    int id = -1;
    for (int i = 0; i < VM_MAX; i++) {
        if (!m->vms[i].in_use) { id = i; break; }
    }
    if (id < 0) return -1;

    VM *v = &m->vms[id];
    memset(v, 0, sizeof(*v));
    v->id = id;
    v->in_use = true;

    if (name && *name) {
        strncpy(v->name, name, sizeof(v->name)-1);
    } else {
        snprintf(v->name, sizeof(v->name), "vm%d", id);
    }

    v->mem = (uint8_t*)calloc(1, ram_bytes);
    if (!v->mem) {
        v->in_use = false;
        return -1;
    }
    v->mem_size = ram_bytes;

    x86_init(&v->cpu, v->mem, v->mem_size);
    v->cpu_inited = true;

    // default start (you can change via set CS/IP)
    v->cpu.cs = 0x0000;
    v->cpu.ip = 0x1000;

    m->current = id;
    return id;
}

static bool vm_use(VMManager *m, int id) {
    VM *v = vm_get(m, id);
    if (!v) return false;
    m->current = id;
    return true;
}

static bool vm_destroy(VMManager *m, int id) {
    VM *v = vm_get(m, id);
    if (!v) return false;
    free(v->mem);
    v->mem = NULL;
    v->mem_size = 0;
    v->cpu_inited = false;
    v->in_use = false;
    v->name[0] = 0;
    if (m->current == id) m->current = -1;
    return true;
}

static void vm_list(VMManager *m) {
    printf("VMs:\n");
    for (int i = 0; i < VM_MAX; i++) {
        VM *v = &m->vms[i];
        if (!v->in_use) continue;
        printf("  %c id=%d name=%s ram=%zu\n",
               (m->current == i) ? '*' : ' ',
               v->id, v->name, v->mem_size);
    }
}

/* Compatibility mode: auto-create default vm on first CPU/mem command */
static VM *ensure_vm(repl_state_t *s) {
    VM *v = vm_current(&s->vmman);
    if (v) return v;

    int id = vm_create_default(&s->vmman, 128u*1024u*1024u, "default");
    if (id < 0) {
        fprintf(stderr, "error: failed to create default VM\n");
        return NULL;
    }
    return vm_current(&s->vmman);
}

/* -----------------------------------------------------------------------------
   load helpers
----------------------------------------------------------------------------- */

static int load_file_to_mem(uint8_t *mem, size_t mem_size, const char *path, uint32_t load_addr) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long len = ftell(f);
    if (len < 0) { fclose(f); return 0; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }

    if ((uint64_t)load_addr + (uint64_t)len > (uint64_t)mem_size) {
        fclose(f);
        return 0;
    }

    size_t n = fread(mem + load_addr, 1, (size_t)len, f);
    fclose(f);
    return n == (size_t)len;
}

/* -----------------------------------------------------------------------------
   regs / trace
----------------------------------------------------------------------------- */

static void print_regs_vm(repl_state_t *s, const VM *vm) {
    const x86_cpu_t *c = &vm->cpu;
    printf("AX=%04X BX=%04X CX=%04X DX=%04X  SI=%04X DI=%04X BP=%04X SP=%04X\n",
           c->ax, c->bx, c->cx, c->dx, c->si, c->di, c->bp, c->sp);
    printf("CS=%04X IP=%04X DS=%04X ES=%04X SS=%04X  FLAGS=%04X\n",
           c->cs, c->ip, c->ds, c->es, c->ss, c->flags);
		   
    // logfile (if open)
    if (s && s->log) {
        log_printf(s, "AX=%04X BX=%04X CX=%04X DX=%04X  SI=%04X DI=%04X BP=%04X SP=%04X",
                   c->ax, c->bx, c->cx, c->dx, c->si, c->di, c->bp, c->sp);
        log_printf(s, "CS=%04X IP=%04X DS=%04X ES=%04X SS=%04X  FLAGS=%04X",
                   c->cs, c->ip, c->ds, c->es, c->ss, c->flags);
    }
}

static x86_status_t step_one_vm(repl_state_t *s, VM *vm) {
    if (s->trace && s->log) {
        uint32_t lin = x86_linear_addr(vm->cpu.cs, vm->cpu.ip);
        uint8_t op = 0;
        if (lin < vm->mem_size) op = vm->mem[lin];
        fprintf(s->log, "%04X:%04X  %02X\n", vm->cpu.cs, vm->cpu.ip, op);
        fflush(s->log);
    }
    return x86_step(&vm->cpu);
}

static int run_steps_vm(repl_state_t *s, VM *vm, uint32_t max_steps) {
    x86_status_t st = X86_OK;
    for (uint32_t i = 0; i < max_steps; i++) {
        st = step_one_vm(s, vm);
        if (st == X86_HALT || st == X86_ERR) break;
    }

    printf("HALT=%d ERR=%d CS:IP=%04X:%04X\n",
           vm->cpu.halted ? 1 : 0,
           (st == X86_ERR) ? 1 : 0,
           vm->cpu.cs, vm->cpu.ip);

    return (st == X86_ERR) ? 1 : 0;
}

/* -----------------------------------------------------------------------------
   command parsing
----------------------------------------------------------------------------- */

static int split_args(char *line, char **argv, int max_args) {
    int argc = 0;
    char *p = line;

    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        if (argc >= max_args) break;

        argv[argc++] = p;

        while (*p && !isspace((unsigned char)*p)) p++;
        if (*p) *p++ = 0;
    }
    return argc;
}

static void log_script_line(repl_state_t *s, const char *path, int line_no, const char *cmd) {
    if (!s || !s->log) return;
    if (!cmd || !*cmd) return;
    log_printf(s, "script:%s:%d> %s", path ? path : "?", line_no, cmd);
}

static int exec_script_file(repl_state_t *s, const char *path, int depth) {
    if (!path || !*path) {
        fprintf(stderr, "usage: do <scriptfile>\n");
        return 1;
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "do: cannot open script: %s\n", path);
        return 1;
    }

    char line[1024];
    int line_no = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_no++;
        trim_right(line);
        char *p = trim_left(line);
        if (*p == 0) continue;

        if (*p == '#' || *p == ';') continue;
        if (p[0] == '/' && p[1] == '/') continue;

        log_script_line(s, path, line_no, p);

        int rc = exec_line(s, p, depth + 1);
        if (rc == 99) { fclose(fp); return 99; }
        /* keep going on rc != 0; logfile shows failing line */
    }

    fclose(fp);
    return 0;
}

static int cmd_examine(VM *vm, uint16_t seg, uint16_t off, size_t count) {
    if (!vm) return 1;
    if (count == 0) return 1;

    uint32_t base = ((uint32_t)seg << 4) + (uint32_t)off;

    if (base >= vm->mem_size) {
        printf("e: address out of range\n");
        return 1;
    }

    if ((size_t)base + count > vm->mem_size)
        count = vm->mem_size - (size_t)base;

    size_t i = 0;
    while (i < count) {
        size_t n = count - i;
        if (n > 16) n = 16;

        printf("%04X:%04X  ", seg, (uint16_t)(off + (uint16_t)i));
        for (size_t j = 0; j < n; j++) {
            printf("%02X ", vm->mem[base + (uint32_t)i + (uint32_t)j]);
        }
        printf("\n");

        i += n;
    }
    return 0;
}

/* wrapper: parses argv strings, calls worker */
static int cmd_examine_args(VM *vm, const char *addr_s, const char *count_s) {
    uint16_t seg = 0, off = 0;
    if (!parse_seg_off(addr_s, &seg, &off)) {
        printf("e: bad address (use ssss:oooo)\n");
        return 1;
    }

    char *end = NULL;
    unsigned long ul = strtoul(count_s, &end, 0);
    if (end == count_s || ul == 0) {
        printf("e: bad count\n");
        return 1;
    }

    return cmd_examine(vm, seg, off, (size_t)ul);
}
	
/* -----------------------------------------------------------------------------
   exec_line
----------------------------------------------------------------------------- */

int exec_line(repl_state_t *s, const char *line_in, int depth) {
    (void)depth;

    char buf[1024];
    strncpy(buf, line_in, sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;

    char *argv[16] = {0};
    int argc = split_args(buf, argv, 16);
    if (argc == 0) return 0;

    const char *cmd = argv[0];

    if (!strcmp(cmd, "help") || !strcmp(cmd, "?")) {
        printf("Commands:\n");
        printf("  logfile <path>\n");
        printf("  set cpu debug=all|on|off\n");
        printf("  version\n");
        printf("  vm create [name] [ram]\n");
        printf("  vm use <id>\n");
        printf("  vm list\n");
        printf("  vm destroy <id>\n");
        printf("  load <bin> <seg:off>\n");
        printf("  set <cs|ip|ds|es|ss|sp> <value>\n");
        printf("  regs\n");
        printf("  run [steps]\n");
        printf("  step [n]\n");
        printf("  quit\n");
        return 0;
    }

	if (!strcmp(cmd, "do")) {
		if (argc < 2) { fprintf(stderr, "usage: do <scriptfile>\n"); return 1; }
		return exec_script_file(s, argv[1], depth + 1);
	}

    if (!strcmp(cmd, "logfile")) {
        if (argc < 2) {
            fprintf(stderr, "usage: logfile <path>\n");
            return 1;
        }
        if (s->log) { fclose(s->log); s->log = NULL; }
        s->log = fopen(argv[1], "w");
        if (!s->log) {
            fprintf(stderr, "error: cannot open logfile: %s\n", argv[1]);
            return 1;
        }
        return 0;
    }

	if (!strcmp(cmd, "e") || !strcmp(cmd, "examine")) {
		if (argc < 3) { printf("usage: e <seg:off> <count>\n"); return 1; }
		VM *vm = ensure_vm(s);
		if (!vm) return 1;
		return cmd_examine_args(vm, argv[1], argv[2]);
	}

    if (!strcmp(cmd, "set") && argc >= 3 && !strcmp(argv[1], "cpu")) {
        if (!strcmp(argv[2], "debug=all") || !strcmp(argv[2], "debug=on")) { s->trace = true; return 0; }
        if (!strcmp(argv[2], "debug=off")) { s->trace = false; return 0; }
        fprintf(stderr, "usage: set cpu debug=all|on|off\n");
        return 1;
    }

    if (!strcmp(cmd, "version")) {
        printf("Version: %s\n", VERSION);
        log_printf(s, "Version: %s", VERSION);
        return 0;
    }

    if (!strcmp(cmd, "vm")) {
        if (argc < 2) { fprintf(stderr, "usage: vm <create|use|list|destroy> ...\n"); return 1; }

        if (!strcmp(argv[1], "list")) {
            vm_list(&s->vmman);
            return 0;
        }
        if (!strcmp(argv[1], "create")) {
            const char *name = (argc >= 3) ? argv[2] : "dummy";
            size_t ram = 128u * 1024u * 1024u;
            if (argc >= 4) {
                int ok = 0;
                size_t v = parse_memsize(argv[3], &ok);
                if (!ok || v < 64u*1024u) { fprintf(stderr, "bad ram size\n"); return 1; }
                ram = v;
            }
            int id = vm_create_default(&s->vmman, ram, name);
            if (id < 0) { fprintf(stderr, "vm create failed\n"); return 1; }
            printf("created vm id=%d (current)\n", id);
            return 0;
        }
        if (!strcmp(argv[1], "use")) {
            if (argc < 3) { fprintf(stderr, "usage: vm use <id>\n"); return 1; }
            int id = atoi(argv[2]);
            if (!vm_use(&s->vmman, id)) { fprintf(stderr, "no such vm id=%d\n", id); return 1; }
            return 0;
        }
        if (!strcmp(argv[1], "destroy")) {
            if (argc < 3) { fprintf(stderr, "usage: vm destroy <id>\n"); return 1; }
            int id = atoi(argv[2]);
            if (!vm_destroy(&s->vmman, id)) { fprintf(stderr, "no such vm id=%d\n", id); return 1; }
            return 0;
        }

        fprintf(stderr, "unknown vm subcommand\n");
        return 1;
    }

    if (!strcmp(cmd, "load")) {
        if (argc < 3) { fprintf(stderr, "usage: load <bin> <seg:off>\n"); return 1; }
        VM *vm = ensure_vm(s);
        if (!vm) return 1;

        uint16_t seg=0, off=0;
        if (!parse_seg_off(argv[2], &seg, &off)) {
            fprintf(stderr, "load: bad address (use ssss:oooo)\n");
            return 1;
        }
        uint32_t addr = x86_linear_addr(seg, off);
        if (!load_file_to_mem(vm->mem, vm->mem_size, argv[1], addr)) {
            fprintf(stderr, "load failed\n");
            return 1;
        }
        return 0;
    }

    if (!strcmp(cmd, "set")) {
        if (argc < 3) {
            fprintf(stderr, "usage: set <cs|ip|ds|es|ss|sp> <value>\n");
            return 1;
        }
        VM *vm = ensure_vm(s);
        if (!vm) return 1;

        uint16_t v=0;
        if (!parse_u16(argv[2], &v)) { fprintf(stderr, "set: bad value\n"); return 1; }

        if (!strcmp(argv[1], "CS") || !strcmp(argv[1], "cs")) { vm->cpu.cs = v; return 0; }
        if (!strcmp(argv[1], "IP") || !strcmp(argv[1], "ip")) { vm->cpu.ip = v; return 0; }
        if (!strcmp(argv[1], "DS") || !strcmp(argv[1], "ds")) { vm->cpu.ds = v; return 0; }
        if (!strcmp(argv[1], "ES") || !strcmp(argv[1], "es")) { vm->cpu.es = v; return 0; }
        if (!strcmp(argv[1], "SS") || !strcmp(argv[1], "ss")) { vm->cpu.ss = v; return 0; }
        if (!strcmp(argv[1], "SP") || !strcmp(argv[1], "sp")) { vm->cpu.sp = v; return 0; }

        fprintf(stderr, "set: unknown reg %s\n", argv[1]);
        return 1;
    }

    if (!strcmp(cmd, "regs")) {
        VM *vm = ensure_vm(s);
        if (!vm) return 1;
        print_regs_vm(s, vm);
        return 0;
    }

    if (!strcmp(cmd, "step")) {
        VM *vm = ensure_vm(s);
        if (!vm) return 1;
        uint32_t n = (argc >= 2) ? (uint32_t)strtoul(argv[1], NULL, 0) : 1u;
        return run_steps_vm(s, vm, n);
    }

    if (!strcmp(cmd, "run")) {
        VM *vm = ensure_vm(s);
        if (!vm) return 1;
        uint32_t n = (argc >= 2) ? (uint32_t)strtoul(argv[1], NULL, 0) : s->default_max_steps;
        return run_steps_vm(s, vm, n);
    }

    if (!strcmp(cmd, "quit") || !strcmp(cmd, "exit")) {
        return 99;
    }

    fprintf(stderr, "unknown command: %s\n", cmd);
    return 1;
}

/* -----------------------------------------------------------------------------
   repl_run
----------------------------------------------------------------------------- */

int repl(Session *sess) {
    repl_state_t s;
    memset(&s, 0, sizeof(s));
	
	(void)sess;   // suppress unused-parameter warning for now

    vmman_init(&s.vmman);

    s.trace = false; // script can enable via: set cpu debug=all
    s.log = NULL;

    char line[1024];
    for (;;) {
        fputs("x64> ", stdout);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;
        trim_right(line);

        char *p = trim_left(line);
        if (*p == 0) continue;

        int rc = exec_line(&s, p, 0);
        if (rc == 99) break;
    }

    if (s.log) fclose(s.log);

    // destroy all vms
    for (int i = 0; i < VM_MAX; i++) {
        if (s.vmman.vms[i].in_use) vm_destroy(&s.vmman, i);
    }
    return 0;
}