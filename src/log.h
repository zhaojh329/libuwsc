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
 
#ifndef _UWSC_LOG_H
#define _UWSC_LOG_H

#include <syslog.h>
#include <string.h>

void uwsc_log_threshold(int threshold);
void uwsc_log_close();

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define uwsc_log(priority, fmt...) __uwsc_log(__FILENAME__, __LINE__, priority, fmt)

#define uwsc_log_debug(fmt...)     uwsc_log(LOG_DEBUG, fmt)
#define uwsc_log_info(fmt...)      uwsc_log(LOG_INFO, fmt)
#define uwsc_log_err(fmt...)       uwsc_log(LOG_ERR, fmt)

void  __uwsc_log(const char *filename, int line, int priority, const char *fmt, ...);

#endif
