/*
 * Copyright (C) 2017 Jianhui Zhao <jianhuizhao329@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "log.h"

static int log_threshold = LOG_DEBUG;
static bool log_initialized;
static const char *ident;

void (*log_write)(int priority, const char *fmt, va_list ap);

static const char *log_ident()
{
    FILE *self;
    static char line[64];
    char *p = NULL;
    char *sbuf;

    if ((self = fopen("/proc/self/status", "r")) != NULL) {
        while (fgets(line, sizeof(line), self)) {
            if (!strncmp(line, "Name:", 5)) {
                strtok_r(line, "\t\n", &sbuf);
                p = strtok_r(NULL, "\t\n", &sbuf);
                break;
            }
        }
        fclose(self);
    }

    return p;
}

static inline void log_write_stdout(int priority, const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
}

static inline void log_write_syslog(int priority, const char *fmt, va_list ap)
{
    vsyslog(priority, fmt, ap);
}

static inline void log_init()
{
    if (log_initialized)
        return;

    ident = log_ident();

    if (isatty(STDOUT_FILENO)) {
        log_write = log_write_stdout;
    } else {
        log_write = log_write_syslog;

        openlog(ident, 0, LOG_DAEMON);
    }

    log_initialized = true;
}


void uwsc_log_threshold(int threshold)
{
    log_threshold = threshold;
}

void uwsc_log_close()
{
    if (!log_initialized)
        return;

    closelog();

    log_initialized = 0;
}

void __uwsc_log(const char *filename, int line, int priority, const char *fmt, ...)
{
    static char new_fmt[256];
    va_list ap;

    if (priority > log_threshold)
        return;

    log_init();

    snprintf(new_fmt, sizeof(new_fmt), "(%s:%d) %s", filename, line, fmt);

    va_start(ap, fmt);
    log_write(priority, new_fmt, ap);
    va_end(ap);
}
