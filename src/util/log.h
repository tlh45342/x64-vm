/* log.h - unified logging/output API (console + optional file, tee, quiet) */
#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

typedef enum log_level {
    LOG_ERROR = 0,
    LOG_WARN  = 1,
    LOG_INFO  = 2,
    LOG_DEBUG = 3,
    LOG_TRACE = 4
} log_level_t;

typedef struct logger {
    FILE *fp;              /* NULL => no file logging */
    bool  tee;             /* if true: console + file */
    bool  quiet;           /* if true: suppress console output */
    log_level_t min_level; /* filter: drop messages below this level */

    /* Optional: prefix behavior */
    bool  show_level;
    bool  show_subsys;
} logger_t;

/* lifecycle */
void logger_init(logger_t *lg);
void logger_shutdown(logger_t *lg);

/* configuration */
void logger_set_level(logger_t *lg, log_level_t lvl);
void logger_set_quiet(logger_t *lg, bool quiet);
void logger_set_tee(logger_t *lg, bool tee);

bool logger_open(logger_t *lg, const char *path, bool tee); /* opens file, replaces existing */
void logger_close(logger_t *lg);

/* emit */
bool logger_enabled(const logger_t *lg, log_level_t lvl);

void log_vprintf(logger_t *lg, log_level_t lvl, const char *subsys,
                 FILE *console, const char *fmt, va_list ap);

void log_printf(logger_t *lg, log_level_t lvl, const char *subsys,
                const char *fmt, ...);

void log_eprintf(logger_t *lg, log_level_t lvl, const char *subsys,
                 const char *fmt, ...);