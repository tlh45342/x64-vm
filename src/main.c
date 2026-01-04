#include "x64_cpu.h"
#include "repl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t parse_u32(const char *s) {
    return (uint32_t)strtoul(s, NULL, 0);
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

int main(int argc, char **argv) {
    // No args -> REPL
    if (argc == 1) {
        return x64_repl_run();
    }

    const char *bin_path = NULL;

    uint32_t load_addr = 0x1000;
    uint16_t cs = 0x0000;
    uint16_t ip = 0x1000;
    uint32_t max_steps = 1000;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--bin") && i + 1 < argc) bin_path = argv[++i];
        else if (!strcmp(argv[i], "--load-addr") && i + 1 < argc) load_addr = parse_u32(argv[++i]);
        else if (!strcmp(argv[i], "--cs") && i + 1 < argc) cs = (uint16_t)parse_u32(argv[++i]);
        else if (!strcmp(argv[i], "--ip") && i + 1 < argc) ip = (uint16_t)parse_u32(argv[++i]);
        else if (!strcmp(argv[i], "--max-steps") && i + 1 < argc) max_steps = parse_u32(argv[++i]);
        else {
            fprintf(stderr,
                "usage:\n"
                "  %s --bin file.bin [--load-addr 0x1000] [--cs 0] [--ip 0x1000] [--max-steps N]\n"
                "  %s            (REPL)\n",
                argv[0], argv[0]);
            return 2;
        }
    }

    if (!bin_path) {
        fprintf(stderr, "error: --bin is required in non-REPL mode\n");
        return 2;
    }

    // Start with 1MB conventional memory; grow later
    size_t mem_size = 1024 * 1024;
    uint8_t *mem = (uint8_t*)calloc(1, mem_size);
    if (!mem) return 1;

    int rc = load_file_to_mem(mem, mem_size, bin_path, load_addr);
    if (rc != 0) {
        fprintf(stderr, "error: failed to load '%s' (rc=%d)\n", bin_path, rc);
        free(mem);
        return 1;
    }

    x86_cpu_t cpu;
    x86_init(&cpu, mem, mem_size);
    cpu.cs = cs;
    cpu.ip = ip;

    x86_status_t st = X86_OK;
    for (uint32_t step = 0; step < max_steps; step++) {
        st = x86_step(&cpu);
        if (st == X86_HALT) break;
        if (st == X86_ERR) break;
    }

    printf("HALT=%d ERR=%d AX=%04x BX=%04x CX=%04x DX=%04x CS:IP=%04x:%04x\n",
           cpu.halted ? 1 : 0,
           (st == X86_ERR) ? 1 : 0,
           cpu.ax, cpu.bx, cpu.cx, cpu.dx,
           cpu.cs, cpu.ip);

    free(mem);
    return (st == X86_ERR) ? 1 : 0;
}
