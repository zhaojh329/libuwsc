/*
 * Copyright (C) 2018 Jianhui Zhao <jianhuizhao329@gmail.com>
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

#ifndef _BUFFER_H
#define _BUFFER_H

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>

/* Test for GCC < 2.96 */
#if __GNUC__ < 2 || (__GNUC__ == 2 && (__GNUC_MINOR__ < 96))
#define __builtin_expect(x) (x)
#endif

#ifndef likely
#define likely(x)	__builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x)	__builtin_expect(!!(x), 0)
#endif

enum {
    P_FD_EOF = 0,
    P_FD_ERR = -1,
    P_FD_PENDING = -2
};

struct buffer {
    size_t persistent; /* persistent size */
    uint8_t *head;  /* Head of buffer */
    uint8_t *data;  /* Data head pointer */
    uint8_t *tail;  /* Data tail pointer */
    uint8_t *end;   /* End of buffer */
};

int buffer_init(struct buffer *b, size_t size);
int buffer_resize(struct buffer *b, size_t size);
void buffer_free(struct buffer *b);


/*  Actual data Length */
static inline size_t buffer_length(const struct buffer *b)
{
    return b->tail - b->data;
}

/* The total buffer size  */
static inline size_t buffer_size(const struct buffer *b)
{
    return b->end - b->head;
}

static inline size_t buffer_headroom(const struct buffer *b)
{
    return b->data - b->head;
}

static inline size_t buffer_tailroom(const struct buffer *b)
{
    return b->end - b->tail;
}

static inline void *buffer_data(const struct buffer *b)
{
    return b->data;
}

static inline void buffer_set_persistent_size(struct buffer *b, size_t size)
{
    size_t new_size = getpagesize();

    while (new_size < size)
        new_size <<= 1;

    b->persistent = new_size;
}

static inline void buffer_check_persistent_size(struct buffer *b)
{
    if (b->persistent > 0 &&
        buffer_size(b) > b->persistent && buffer_length(b) < b->persistent)
        buffer_resize(b, b->persistent);
}

void *buffer_put(struct buffer *b, size_t len);

static inline void *buffer_put_zero(struct buffer *b, size_t len)
{
    void *tmp = buffer_put(b, len);

    if (likely(tmp))
        memset(tmp, 0, len);
    return tmp;
}

static inline void *buffer_put_data(struct buffer *b, const void *data,   size_t len)
{
    void *tmp = buffer_put(b, len);

    if (likely(tmp))
        memcpy(tmp, data, len);
    return tmp;
}


static inline int buffer_put_u8(struct buffer *b, uint8_t val)
{
    uint8_t *p = buffer_put(b, 1);

    if (likely(p)) {
        *p = val;
        return 0;
    }

    return -1;
}

static inline int buffer_put_u16(struct buffer *b, uint16_t val)
{
    uint16_t *p = buffer_put(b, 2);

    if (likely(p)) {
        *p = val;
        return 0;
    }

    return -1;
}

static inline int buffer_put_u32(struct buffer *b, uint32_t val)
{
    uint32_t *p = buffer_put(b, 4);

    if (likely(p)) {
        *p = val;
        return 0;
    }

    return -1;
}

static inline int buffer_put_u64(struct buffer *b, uint64_t val)
{
    uint64_t *p = buffer_put(b, 8);

    if (likely(p)) {
        *p = val;
        return 0;
    }

    return -1;
}

static inline int buffer_put_string(struct buffer *b, const char *s)
{
    size_t len = strlen(s);
    char *p = buffer_put(b, len);

    if (likely(p)) {
        memcpy(p, s, len);
        return 0;
    }

    return -1;
}

int buffer_put_vprintf(struct buffer *b, const char *fmt, va_list ap) __attribute__((format(printf, 2, 0)));
int buffer_put_printf(struct buffer *b, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

int buffer_put_fd(struct buffer *b, int fd, ssize_t len, bool *eof,
    int (*rd)(int fd, void *buf, size_t count, void *arg), void *arg);

/**
 *	buffer_truncate - remove end from a buffer
 *	@b: buffer to alter
 *	@len: new length
 *
 *	Cut the length of a buffer down by removing data from the tail. If
 *	the buffer is already under the length specified it is not modified.
 */
static inline void buffer_truncate(struct buffer *b, size_t len)
{
    if (buffer_length(b) > len) {
        b->tail = b->data + len;
        buffer_check_persistent_size(b);
    }
}


size_t buffer_pull(struct buffer *b, void *dest, size_t len);

static inline uint8_t buffer_pull_u8(struct buffer *b)
{
    uint8_t val = 0;

    if (likely(buffer_length(b) > 0)) {
        val = b->data[0];
        b->data += 1;
    }

    return val;
}

static inline uint16_t buffer_pull_u16(struct buffer *b)
{
    uint16_t val = 0;

    if (likely(buffer_length(b) > 1)) {
        val = *((uint16_t *)b->data);
        b->data += 2;
    }

    return val;
}

static inline uint32_t buffer_pull_u32(struct buffer *b)
{
    uint32_t val = 0;

    if (likely(buffer_length(b) > 3)) {
        val = *((uint32_t *)b->data);
        b->data += 4;
    }

    return val;
}

static inline uint64_t buffer_pull_u64(struct buffer *b)
{
    uint64_t val = 0;

    if (likely(buffer_length(b) > 7)) {
        val = *((uint64_t *)b->data);
        b->data += 8;
    }

    return val;
}

static inline uint8_t buffer_get_u8(struct buffer *b, ssize_t offset)
{
    uint8_t val = 0;

    if (likely(buffer_length(b) > offset))
        val = b->data[offset];

    return val;
}

static inline uint16_t buffer_get_u16(struct buffer *b, ssize_t offset)
{
    uint16_t val = 0;

    if (likely(buffer_length(b) > offset + 1))
        val = *((uint16_t *)(b->data + offset));

    return val;
}

static inline uint32_t buffer_get_u32(struct buffer *b, ssize_t offset)
{
    uint32_t val = 0;

    if (likely(buffer_length(b) > offset + 3))
        val = *((uint32_t *)(b->data + offset));

    return val;
}

static inline uint64_t buffer_get_u64(struct buffer *b, ssize_t offset)
{
    uint64_t val = 0;

    if (likely(buffer_length(b) > offset + 7))
        val = *((uint64_t *)(b->data + offset));

    return val;
}

int buffer_pull_to_fd(struct buffer *b, int fd, size_t len,
    int (*wr)(int fd, void *buf, size_t count, void *arg), void *arg);

void buffer_hexdump(struct buffer *b, size_t offset, size_t len);

#endif
