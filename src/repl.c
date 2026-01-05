// src/repl.c

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include "repl.h"
#include "version.h"
#include "cpu/cpu.h"

#include "vm.h"

static VMManager g_vmman;

typedef struct repl_state {
    VMManager *vmman;

    char img_path[512];
    uint32_t default_max_steps;

    bool trace;      // if true, log each step (CS:IP opcode)
    FILE *log;       // may be NULL
} repl_state_t;   // may be NULL
} repl_state_t;

static VM *cur_vm(repl_state_t *s) {
    return vm_current(s->vmman);
}

// Compatibility mode: if no VM exists yet, auto-create one using defaults.
// This preserves your existing scripts that don’t mention "vm create".
static VM *ensure_vm(repl_state_t *s) {
    VM *v = cur_vm(s);
    if (v) return v;

    // Default: 128MB, name "default"
    int id = vm_create_default(s->vmman, 128u * 1024u * 1024u, "default");
    if (id < 0) {
        fprintf(stderr, "error: failed to auto-create default VM\n");
        return NULL;
    }
    return cur_vm(s);
}


static void logf_repl(repl_state_t *s, const char *fmt, ...) __attribute__((unused));

static void trim_right(char *s) {
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1]))) {
        s[--n] = 0;
    }
}

static char *trim_left(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static int is_space(char c) { return (c==' ' || c=='\t' || c=='\r' || c=='\n'); }

static const char *skip_ws(const char *p) {
    while (*p && is_space(*p)) p++;
    return p;
}

static int parse_u16(const char *s, uint16_t *out) {
    // accepts: 0x1234, 1234 (hex), 1234d (decimal not supported), or plain decimal if no hex letters
    // simplest: use strtoul base 0 (0x => hex), otherwise decimal.
    // But for your world, bare "1000" is usually hex. We'll treat bare as hex if it contains A-F.
    char *end = NULL;

    // trim leading ws
    s = skip_ws(s);
    if (!*s) return 0;

    // heuristic: if starts with 0x => base 0, else if contains hex letter => base 16, else base 10
    int base = 0;
    if (!(s[0]=='0' && (s[1]=='x' || s[1]=='X'))) {
        for (const char *p = s; *p; p++) {
            char c = *p;
            if (is_space(c)) break;
            if ((c>='a'&&c<='f') || (c>='A'&&c<='F')) { base = 16; break; }
        }
        if (base == 0) base = 10;
    }

    unsigned long v = strtoul(s, &end, base);
    if (end == s) return 0;                 // no parse
    if (v > 0xFFFFul) return 0;
    *out = (uint16_t)v;
    return 1;
}

static int parse_seg_off(const char *s, uint16_t *seg, uint16_t *off) {
    // Accept "ssss:oooo" (hex). Allow "0:1000" too.
    unsigned long a = 0, b = 0;
    char *end = NULL;

    if (!s || !*s) return 0;

    a = strtoul(s, &end, 16);
    if (end == s || *end != ':') return 0;
    s = end + 1;

    b = strtoul(s, &end, 16);
    if (end == s || *end != '\0') return 0;

    if (a > 0xFFFFul || b > 0xFFFFul) return 0;
    *seg = (uint16_t)a;
    *off = (uint16_t)b;
    return 1;
}

static int load_bin_into_mem(x86_cpu_t *cpu, const char *path, uint16_t seg, uint16_t off) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "load: cannot open '%s'\n", path);
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 1; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 1; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 1; }

    uint32_t lin = x86_linear_addr(seg, off);
    if (lin + (uint32_t)sz > cpu->mem_size) {
        fprintf(stderr, "load: image too large: %ld bytes at %04x:%04x (phys 0x%08X) exceeds mem\n",
                sz, seg, off, lin);
        fclose(f);
        return 1;
    }

    size_t nread = fread(cpu->mem + lin, 1, (size_t)sz, f);
    fclose(f);

    if (nread != (size_t)sz) {
        fprintf(stderr, "load: short read '%s'\n", path);
        return 1;
    }

    // Log line that tests can match
    printf("[LOAD] %s seg:off=%04x:%04x phys=0x%08X size=%ld\n", path, seg, off, lin, sz);
    return 0;
}


// Very small argv splitter with quotes.
// Returns argc, writes pointers into argv[] (which point into 'line' buffer).
static int split_args(char *line, char **argv, int max_argv) {
    int argc = 0;
    char *p = trim_left(line);

    while (*p && argc < max_argv) {
        // comment-only at start of token
        if (*p == '#') break;
        if (*p == ';') break;
        if (p[0] == '/' && p[1] == '/') break;

        // token
        if (*p == '"') {
            p++; // skip opening quote
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = 0;
        } else {
            argv[argc++] = p;
            while (*p && !isspace((unsigned char)*p)) p++;
            if (*p) *p++ = 0;
        }

        p = trim_left(p);
    }

    return argc;
}

static void logf_repl(repl_state_t *s, const char *fmt, ...) {
    if (!s->log) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(s->log, fmt, ap);
    va_end(ap);
    fflush(s->log);
}

static void log_printf(repl_state_t *s, const char *fmt, ...)
{
    if (!s->log) return;

    va_list ap;
    va_start(ap, fmt);
    vfprintf(s->log, fmt, ap);
    va_end(ap);

    fputc('\n', s->log);
    fflush(s->log);
}

static int load_file_to_mem(uint8_t *mem, size_t mem_size, const char *path, uint32_t load_addr) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -2; }
    long len = ftell(f);
    if (len < 0) { fclose(f); return -2; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return -2; }

    if ((uint64_t)load_addr + (uint64_t)len > (uint64_t)mem_size) {
        fclose(f);
        return -3;
    }

    size_t n = fread(mem + load_addr, 1, (size_t)len, f);
    fclose(f);
    return (n == (size_t)len) ? 0 : -4;
}

// returns: 1 booted, 0 not found, -1 error
static int load_boot_sector_7c00(uint8_t *mem, size_t memsz, const char *img_path) {
    FILE *f = fopen(img_path, "rb");
    if (!f) return 0; // not found / can't open

    if (memsz < 0x7C00u + 512u) { fclose(f); return -1; }

    size_t n = fread(mem + 0x7C00u, 1, 512u, f);
    fclose(f);

    return (n == 512u) ? 1 : -1;
}

// static void print_regs(const repl_state_t *s) {
//     const x86_cpu_t *c = &s->cpu;
//     printf("AX=%04x BX=%04x CX=%04x DX=%04x  SP=%04x BP=%04x SI=%04x DI=%04x\n",
//            c->ax, c->bx, c->cx, c->dx, c->sp, c->bp, c->si, c->di);
//     printf("CS=%04x DS=%04x ES=%04x SS=%04x  IP=%04x FLAGS=%04x  HALT=%d\n",
//            c->cs, c->ds, c->es, c->ss, c->ip, c->flags, c->halted ? 1 : 0);
// }

static int parse_seg_off_hex(const char *s, uint16_t *seg, uint16_t *off) {
    unsigned long a = 0, b = 0;
    char *end = NULL;

    if (!s || !*s) return 0;

    a = strtoul(s, &end, 16);
    if (end == s || *end != ':') return 0;

    b = strtoul(end + 1, &end, 16);
    if (end == s || *end != '\0') return 0;

    if (a > 0xFFFFul || b > 0xFFFFul) return 0;
    *seg = (uint16_t)a;
    *off = (uint16_t)b;
    return 1;
}

static void dump_bytes_vm(VM *vm, uint16_t seg, uint16_t off, unsigned count) {
    if (count == 0) count = 16;
    if (count > 256) count = 256;

    uint32_t lin = x86_linear_addr(seg, off);

    printf("%04x:%04x  ", seg, off);

    for (unsigned i = 0; i < count; i++) {
        uint32_t a = lin + i;
        if (a >= vm->mem_size) printf("?? ");
        else printf("%02x ", vm->mem[a]);
    }
    printf("\n");
}

static void reset_cpu(repl_state_t *s, uint16_t cs, uint16_t ip) {
    // keep memory as-is; reset registers/flags
    memset(&s->cpu, 0, sizeof(s->cpu));
    x86_init(&s->cpu, s->mem, s->mem_size);
    s->cpu.cs = cs;
    s->cpu.ip = ip;
}

static x86_status_t step_one(repl_state_t *s) {
    // Optional per-instruction trace (raw opcode only for now)
    if (s->trace && s->log) {
        uint32_t lin = x86_linear_addr(s->cpu.cs, s->cpu.ip);
        uint8_t op = 0x00;
        if (lin < s->mem_size) op = s->mem[lin];
        fprintf(s->log, "%04x:%04x  %02x\n", s->cpu.cs, s->cpu.ip, op);
        fflush(s->log);
    }
    return x86_step(&s->cpu);
}

static int run_steps(repl_state_t *s, uint32_t max_steps) {
    x86_status_t st = X86_OK;
    for (uint32_t i = 0; i < max_steps; i++) {
        st = step_one(s);
        if (st == X86_HALT) break;
        if (st == X86_ERR) break;
    }

    printf("HALT=%d ERR=%d AX=%04x BX=%04x CX=%04x DX=%04x CS:IP=%04x:%04x\n",
           s->cpu.halted ? 1 : 0,
           (st == X86_ERR) ? 1 : 0,
           s->cpu.ax, s->cpu.bx, s->cpu.cx, s->cpu.dx,
           s->cpu.cs, s->cpu.ip);

    return (st == X86_ERR) ? 1 : 0;
}

static void print_help(void) {
    puts(
        "commands:\n"
        "  help                     show this help\n"
        "  version                  print version\n"
        "  regs                     print registers\n"
        "  trace on|off             per-instruction trace to logfile (default: on)\n"
        "  logfile <path>           set logfile (default: x64vm.log)\n"
        "  reset [cs:ip]            reset CPU (default 0000:1000)\n"
        "  loadbin <file> [addr] [cs:ip]\n"
        "                           load flat binary into memory\n"
        "  boot [img]               read sector0 from img -> 0000:7C00 and set CS:IP\n"
        "  step [n]                 execute n steps (default 1)\n"
        "  run [n]                  run n steps (default configured)\n"
        "  do <script>              execute commands from a script\n"
        "  quit/exit                leave\n"
    );
}

static int exec_line(repl_state_t *s, const char *line_in, int depth);

static int do_script(repl_state_t *s, const char *path, int depth) {
    if (depth > 16) {
        fprintf(stderr, "do: recursion too deep\n");
        return 1;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "do: cannot open '%s'\n", path);
        return 1;
    }

    char buf[1024];
    int rc = 0;
    int lineno = 0;
    while (fgets(buf, sizeof(buf), f)) {
        lineno++;
        trim_right(buf);
        char *p = trim_left(buf);
        if (*p == 0) continue;
        if (*p == '#') continue;
        if (*p == ';') continue;
        if (p[0] == '/' && p[1] == '/') continue;

        rc = exec_line(s, p, depth + 1);
        if (rc != 0) {
            fprintf(stderr, "do: %s:%d failed\n", path, lineno);
            break;
        }
    }
    fclose(f);
    return rc;
}

static int set_logfile(repl_state_t *s, const char *path) {
    if (s->log) {
        fclose(s->log);
        s->log = NULL;
    }
    s->log = fopen(path, "w");
    if (!s->log) {
        fprintf(stderr, "logfile: failed to open '%s'\n", path);
        return 1;
    }
    fflush(s->log);
    return 0;
}

static x86_status_t step_one_vm(repl_state_t *s, VM *vm) {
    if (s->trace && s->log) {
        uint32_t lin = x86_linear_addr(vm->cpu.cs, vm->cpu.ip);
        uint8_t op = 0x00;
        if (lin < vm->mem_size) op = vm->mem[lin];
        fprintf(s->log, "%04x:%04x  %02x\n", vm->cpu.cs, vm->cpu.ip, op);
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

    printf("HALT=%d ERR=%d AX=%04x BX=%04x CX=%04x DX=%04x CS:IP=%04x:%04x\n",
           vm->cpu.halted ? 1 : 0,
           (st == X86_ERR) ? 1 : 0,
           vm->cpu.ax, vm->cpu.bx, vm->cpu.cx, vm->cpu.dx,
           vm->cpu.cs, vm->cpu.ip);

    return (st == X86_ERR) ? 1 : 0;
}


static int regs_to_string_vm(const VM *vm, char *buf, size_t cap)
{
    const x86_cpu_t *c = &vm->cpu;

    return snprintf(buf, cap,
        "AX=%04X BX=%04X CX=%04X DX=%04X  SI=%04X DI=%04X BP=%04X SP=%04X\n"
        "CS=%04X IP=%04X DS=%04X ES=%04X SS=%04X  FLAGS=%04X",
        c->ax, c->bx, c->cx, c->dx,
        c->si, c->di, c->bp, c->sp,
        c->cs, c->ip, c->ds, c->es, c->ss,
        c->flags
    );
}

int exec_line(repl_state_t *s, const char *line_in, int depth) {
    (void)depth;

    char line[1024];
    strncpy(line, line_in, sizeof(line) - 1);
    line[sizeof(line) - 1] = 0;

    char *argv[16];
    int argc = split_args(line, argv, 16);
    if (argc == 0) return 0;

    const char *cmd = argv[0];

if (!strcmp(cmd, "set")) {
    // set cpu debug=all|on|off
    if (argc >= 3 && !strcmp(argv[1], "cpu")) {
        if (!strcmp(argv[2], "debug=all") || !strcmp(argv[2], "debug=on")) { s->trace = true; return 0; }
        if (!strcmp(argv[2], "debug=off")) { s->trace = false; return 0; }
        fprintf(stderr, "usage: set cpu debug=all|on|off\n");
        return 1;
    }

    // set <reg> <value>  OR set reg=value
    if (argc < 3) {
        fprintf(stderr, "usage: set <cs|ip|ds|es|ss|sp> <value>\n");
        fprintf(stderr, "   or: set <name>=<value>\n");
        return 1;
    }

    VM *vm = ensure_vm(s);
    if (!vm) return 1;

    const char *name = argv[1];
    const char *valstr = argv[2];

    const char *eq = strchr(name, '=');
    char namebuf[32];
    if (eq) {
        size_t n = (size_t)(eq - name);
        if (n >= sizeof(namebuf)) n = sizeof(namebuf)-1;
        memcpy(namebuf, name, n);
        namebuf[n] = 0;
        name = namebuf;
        valstr = eq + 1;
    }

    uint16_t v = 0;
    if (!parse_u16(valstr, &v)) {
        fprintf(stderr, "set: bad value: %s\n", valstr);
        return 1;
    }

    if (!strcmp(name, "cs")) { vm->cpu.cs = v; return 0; }
    if (!strcmp(name, "ip")) { vm->cpu.ip = v; return 0; }
    if (!strcmp(name, "ds")) { vm->cpu.ds = v; return 0; }
    if (!strcmp(name, "es")) { vm->cpu.es = v; return 0; }
    if (!strcmp(name, "ss")) { vm->cpu.ss = v; return 0; }
    if (!strcmp(name, "sp")) { vm->cpu.sp = v; return 0; }

    fprintf(stderr, "set: unknown field: %s\n", name);
    return 1;
}


    if (!strcmp(cmd, "help") || !strcmp(cmd, "?")) {
        print_help();
        return 0;
    }

	if (!strcmp(cmd, "e") || !strcmp(cmd, "x")) {
		if (argc < 2) {
			fprintf(stderr, "usage: e <seg:off> [count]\n");
			return 1;
		}

		uint16_t seg = 0, off = 0;
		if (!parse_seg_off_hex(argv[1], &seg, &off)) {
			fprintf(stderr, "e: bad address '%s' (expected ssss:oooo hex)\n", argv[1]);
			return 1;
		}

		unsigned count = 16;
		if (argc >= 3) {
			count = (unsigned)strtoul(argv[2], NULL, 0);
			if (count == 0) count = 16;
		}

		dump_bytes(&s->cpu, seg, off, count);
		return 0;
	}

    if (!strcmp(cmd, "version")) {
        printf("x64-vm: %s\n", X64VM_VERSION);
		log_printf(s, "Version: %s", X64VM_VERSION);
        return 0;
    }

	if (!strcmp(cmd, "regs")) {
		char tmp[256];
		regs_to_string(s, tmp, sizeof(tmp));

		// show it to the user
		fputs(tmp, stdout);

		// optionally also log it
		log_printf(s, "%s", tmp);

		return 0;
	}

    if (!strcmp(cmd, "trace")) {
        if (argc < 2) {
            printf("trace is %s\n", s->trace ? "on" : "off");
            return 0;
        }
        if (!strcmp(argv[1], "on")) s->trace = true;
        else if (!strcmp(argv[1], "off")) s->trace = false;
        else fprintf(stderr, "usage: trace on|off\n");
        return 0;
    }

	if (!strcmp(cmd, "logfile")) {
		if (argc < 2) {
			fprintf(stderr, "usage: logfile <path>\n");
			return 1;
		}
		return set_logfile(s, argv[1]);
	}

	if (!strcmp(cmd, "load")) {
		if (argc < 2) {
			fprintf(stderr, "usage: load <file.bin> <seg:off>\n");
			return 1;
		}
		if (argc < 3) {
			fprintf(stderr, "load: missing address. required form is seg:off (e.g. 0000:1000)\n");
			return 1;
		}

		uint16_t seg = 0, off = 0;
		if (!parse_seg_off(argv[2], &seg, &off)) {
			fprintf(stderr, "load: bad address '%s' (expected ssss:oooo hex)\n", argv[2]);
			return 1;
		}

		return load_bin_into_mem(&s->cpu, argv[1], seg, off);
	}

    if (!strcmp(cmd, "reset")) {
        uint16_t cs = 0x0000, ip = 0x1000;
        if (argc >= 2) {
            unsigned seg = 0, off = 0;
            if (sscanf(argv[1], "%x:%x", &seg, &off) == 2) {
                cs = (uint16_t)seg;
                ip = (uint16_t)off;
            } else {
                fprintf(stderr, "reset: expected cs:ip\n");
                return 1;
            }
        }
        reset_cpu(s, cs, ip);
        return 0;
    }

    if (!strcmp(cmd, "boot")) {
        const char *img = (argc >= 2) ? argv[1] : s->img_path;

        int ok = load_boot_sector_7c00(s->mem, s->mem_size, img);
        if (ok == 1) {
            reset_cpu(s, 0x0000, 0x7C00);
            printf("booted sector0 from '%s' at 0000:7C00\n", img);
            return 0;
        }
        if (ok == 0) {
            fprintf(stderr, "boot: cannot open '%s'\n", img);
            return 1;
        }
        fprintf(stderr, "boot: failed to read boot sector from '%s'\n", img);
        return 1;
    }

    if (!strcmp(cmd, "step")) {
        uint32_t n = (argc >= 2) ? (uint32_t)strtoul(argv[1], NULL, 0) : 1u;
        return run_steps(s, n);
    }

    if (!strcmp(cmd, "run")) {
        uint32_t n = (argc >= 2) ? (uint32_t)strtoul(argv[1], NULL, 0) : s->default_max_steps;
        return run_steps(s, n);
    }

    if (!strcmp(cmd, "do")) {
        if (argc < 2) {
            fprintf(stderr, "usage: do <script>\n");
            return 1;
        }
        return do_script(s, argv[1], depth);
    }

    if (!strcmp(cmd, "quit") || !strcmp(cmd, "exit")) {
        return 99; // signal to exit repl
    }

    fprintf(stderr, "unknown command: %s (try 'help')\n", cmd);
    return 1;
}

int repl_run(void) {
    repl_state_t s;
    memset(&s, 0, sizeof(s));
	
	vmman_init(&g_vmman);

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

    return 0;
}