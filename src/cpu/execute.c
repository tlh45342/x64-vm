// src/cpu/execute.c

x86_status_t cpu_execute(exec_ctx_t *e) {
    // 1) trace at CS:IP start
    // 2) decode into e->d (or into fields on exec_ctx)
    // 3) fn = x86_decode_ctx(e)   (table dispatch)
    // 4) fn(e)
}