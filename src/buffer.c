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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

#include "buffer.h"

static int buffer_resize(struct buffer *b, size_t size)
{
    uint8_t *head;
    size_t new_size = getpagesize();
    int data_len = buffer_length(b);

    while (new_size < size)
        new_size <<= 1;

    if (likely(b->head)) {
        if (buffer_headroom(b) > 0) {
            memmove(b->head, b->data, data_len);
            b->data = b->head;
            b->tail = b->data + data_len;
        }

        head = realloc(b->head, new_size);
    } else {
        head = malloc(new_size);
    }

    if (unlikely(!head))
        return -1;

    b->head = b->data = head;
    b->tail = b->data + data_len;
    b->end = b->head + new_size;

    if (unlikely(b->tail > b->end))
        b->tail = b->end;

    return 0;
}

int buffer_init(struct buffer *b, size_t size)
{
    memset(b, 0, sizeof(struct buffer));

    if (size)
        return buffer_resize(b, size);

    return 0;
}

void buffer_free(struct buffer *b)
{
    if (b->head) {
        free(b->head);
        memset(b, 0, sizeof(struct buffer));
    }
}

static inline int buffer_grow(struct buffer *b, size_t len)
{
    return buffer_resize(b, buffer_size(b) + len);
}

/**
 *	buffer_put - add data to a buffer
 *	@b: buffer to use
 *	@len: amount of data to add
 *
 *	This function extends the used data area of the buffer. A pointer to the
 *	first byte of the extra data is returned.
 *  If this would exceed the total buffer size the buffer will grow automatically.
 */
void *buffer_put(struct buffer *b, size_t len)
{
    void *tmp;

    if (buffer_length(b) == 0)
        b->tail = b->data = b->head;

    if (buffer_tailroom(b) < len && buffer_grow(b, len) < 0)
        return NULL;

    tmp = b->tail;
    b->tail += len;
    return tmp;
}

int buffer_put_vprintf(struct buffer *b, const char *fmt, va_list ap)
{
    for (;;) {
        int ret;
        va_list local_ap;
        size_t tail_room = buffer_tailroom(b);

        va_copy(local_ap, ap);
        ret = vsnprintf((char *)b->tail, tail_room, fmt, local_ap);
        va_end(local_ap);

        if (ret < 0)
            return -1;

        if (likely(ret < tail_room)) {
            b->tail += ret;
            return 0;
        }

        if (unlikely(buffer_grow(b, 1) < 0))
            return -1;
    }
}

int buffer_put_printf(struct buffer *b, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = buffer_put_vprintf(b, fmt, ap);
    va_end(ap);

    return ret;
}

/*
*  buffer_put_fd - Append data from a file to the end of a buffer
*  @fd: file descriptor
*  @len: how much data to read, or -1 to read as much as possible.
*  @eof: indicates end of file
*  @rd: A customized read function. Generally used for SSL.
*       The customized read function should be return:
*       P_FD_EOF/P_FD_ERR/P_FD_PENDING or number of bytes read.
*
*  Return the number of bytes append
*/
int buffer_put_fd(struct buffer *b, int fd, ssize_t len, bool *eof,
    int (*rd)(int fd, void *buf, size_t count, void *arg), void *arg)
{
    ssize_t remain;

    if (len < 0)
        len = INT_MAX;

    remain = len;
    *eof = false;

    do {
        ssize_t ret;
        size_t tail_room = buffer_tailroom(b);

        if (unlikely(!tail_room)) {
            if (buffer_grow(b, 1) < 0)
                return -1;
            tail_room = buffer_tailroom(b);
        }

        if (rd) {
            ret = rd(fd, b->tail, tail_room, arg);
            if (ret == P_FD_ERR)
                return -1;
            else if (ret == P_FD_PENDING)
                break;
        } else {
            ret = read(fd, b->tail, tail_room);
            if (unlikely(ret < 0)) {
                if (errno == EINTR)
                    continue;

                if (errno == EAGAIN || errno == ENOTCONN)
                    break;

                return -1;
            }
        }

        if (!ret) {
            *eof = true;
            break;
        }

        b->tail += ret;
        remain -= ret;
    } while (remain);

    return len - remain;
}

/**
 *	buffer_pull - remove data from the start of a buffer
 *	@b: buffer to use
 *	@len: amount of data to remove
 *
 *	This function removes data from the start of a buffer,
 *  returning the actual length removed.
 *  Just remove the data if the dest is NULL.
 */
size_t buffer_pull(struct buffer *b, void *dest, size_t len)
{
    if (len > buffer_length(b))
        len = buffer_length(b);

    if (dest)
        memcpy(dest, b->data, len);

    b->data += len;

    return len;
}

/*
*  buffer_pull_to_fd - remove data from the start of a buffer and write to a file
*  @fd: file descriptor
*  @len: how much data to remove, or -1 to remove as much as possible.
*  @wr: A customized write function. Generally used for SSL.
*       The customized write function should be return:
*       P_FD_EOF/P_FD_ERR/P_FD_PENDING or number of bytes write.
*
*  Return the number of bytes removed
*/
int buffer_pull_to_fd(struct buffer *b, int fd, size_t len,
    int (*wr)(int fd, void *buf, size_t count, void *arg), void *arg)
{
    ssize_t remain;

    if (len < 0)
        len = INT_MAX;

    remain = len;

    do {
        ssize_t ret;
        size_t data_len = buffer_length(b);

        if (!data_len)
            break;

        if (wr) {
            ret = wr(fd, b->data, data_len, arg);
            if (ret == P_FD_ERR)
                return -1;
            else if (ret == P_FD_PENDING)
                break;
        } else {
            ret = write(fd, b->data, data_len);
            if (ret < 0) {
                if (errno == EINTR)
                    continue;

                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOTCONN)
                    break;

                return -1;
            }
        }

        remain -= ret;
        b->data += ret;
    } while (remain);

    return len - remain;
}

void buffer_hexdump(struct buffer *b, size_t offset, size_t len)
{
    int i;
    size_t data_len = buffer_length(b);
    uint8_t *data = buffer_data(b);

    if (offset > data_len - 1)
        return;

    if (len > data_len)
        len = data_len;

    for (i = offset; i < len; i++) {
        printf("%02X ", data[i]);
        if (i && i % 16 == 0)
            printf("\n");
    }
    printf("\n");
}