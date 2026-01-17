// src/log.c
#include "log.h"

#include <string.h>
#include <stdarg.h>

static const char *lvl_name(log_level_t lvl) {
    switch (lvl) {
        case LOG_ERROR: return "ERROR";
        case LOG_WARN:  return "WARN";
        case LOG_INFO:  return "INFO";
        case LOG_DEBUG: return "DEBUG";
        case LOG_TRACE: return "TRACE";
        default:        return "LOG";
    }
}

void logger_init(logger_t *lg) {
    if (!lg) return;
    memset(lg, 0, sizeof(*lg));
    lg->fp = NULL;
    lg->tee = true;
    lg->quiet = false;
    lg->min_level = LOG_INFO;
    lg->show_level = false;
    lg->show_subsys = false;
}

void logger_shutdown(logger_t *lg) {
    if (!lg) return;
    logger_close(lg);
}

void logger_set_level(logger_t *lg, log_level_t lvl) { if (lg) lg->min_level = lvl; }
void logger_set_quiet(logger_t *lg, bool quiet)      { if (lg) lg->quiet = quiet; }
void logger_set_tee(logger_t *lg, bool tee)          { if (lg) lg->tee = tee; }

bool logger_open(logger_t *lg, const char *path, bool tee) {
    if (!lg || !path || !*path) return false;
    logger_close(lg);

    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    lg->fp = fp;
    lg->tee = tee;
    return true;
}

void logger_close(logger_t *lg) {
    if (!lg) return;
    if (lg->fp) {
        fclose(lg->fp);
        lg->fp = NULL;
    }
}

/* lower number = more severe. enabled if lvl <= min_level */
bool logger_enabled(const logger_t *lg, log_level_t lvl) {
    if (!lg) return false;
    return (lvl <= lg->min_level);
}

static void emit_prefix(logger_t *lg, FILE *out, log_level_t lvl, const char *subsys) {
    if (!lg || !out) return;
    if (lg->show_level) {
        fprintf(out, "[%s]", lvl_name(lvl));
        if (lg->show_subsys && subsys && *subsys) fprintf(out, "[%s] ", subsys);
        else fprintf(out, " ");
    } else if (lg->show_subsys && subsys && *subsys) {
        fprintf(out, "[%s] ", subsys);
    }
}

void log_vprintf(logger_t *lg, log_level_t lvl, const char *subsys,
                 FILE *console, const char *fmt, va_list ap_in)
{
    if (!lg) return;
    if (!logger_enabled(lg, lvl)) return;

    const bool have_file   = (lg->fp != NULL);
    const bool do_console  = (!lg->quiet);

    va_list ap1, ap2;
    va_copy(ap1, ap_in);
    va_copy(ap2, ap_in);

    if (do_console) {
        emit_prefix(lg, console, lvl, subsys);
        vfprintf(console, fmt, ap1);
        fflush(console);
    }

    if (have_file && (lg->tee || !do_console)) {
        emit_prefix(lg, lg->fp, lvl, subsys);
        vfprintf(lg->fp, fmt, ap2);
        fflush(lg->fp);
    }

    va_end(ap1);
    va_end(ap2);
}

void log_printf(logger_t *lg, log_level_t lvl, const char *subsys,
                const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vprintf(lg, lvl, subsys, stdout, fmt, ap);
    va_end(ap);
}

void log_eprintf(logger_t *lg, log_level_t lvl, const char *subsys,
                 const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vprintf(lg, lvl, subsys, stderr, fmt, ap);
    va_end(ap);
}
