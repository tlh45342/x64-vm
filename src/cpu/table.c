// src/cpu/table.c

x86_fn_t x86_decode_ctx(exec_ctx_t *e) {
    // parse prefixes, fetch opcode (and 0F if needed)
    // consult table[op]
    // decode modrm/imm/disp if table says so
    // return handler
}