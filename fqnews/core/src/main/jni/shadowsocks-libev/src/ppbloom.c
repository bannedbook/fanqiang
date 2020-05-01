/*
 * ppbloom.c - Ping-Pong Bloom Filter for nonce reuse detection
 *
 * Copyright (C) 2013 - 2019, Max Lv <max.c.lv@gmail.com>
 *
 * This file is part of the shadowsocks-libev.
 *
 * shadowsocks-libev is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * shadowsocks-libev is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shadowsocks-libev; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdlib.h>

#include "bloom.h"
#include "ppbloom.h"
#include "utils.h"

#define PING 0
#define PONG 1

static struct bloom ppbloom[2];
static int bloom_count[2];
static int current;
static int entries;
static double error;

int
ppbloom_init(int n, double e)
{
    int err;
    entries = n / 2;
    error   = e;

    err = bloom_init(ppbloom + PING, entries, error);
    if (err)
        return err;

    err = bloom_init(ppbloom + PONG, entries, error);
    if (err)
        return err;

    bloom_count[PING] = 0;
    bloom_count[PONG] = 0;

    current = PING;

    return 0;
}

int
ppbloom_check(const void *buffer, int len)
{
    int ret;

    ret = bloom_check(ppbloom + PING, buffer, len);
    if (ret)
        return ret;

    ret = bloom_check(ppbloom + PONG, buffer, len);
    if (ret)
        return ret;

    return 0;
}

int
ppbloom_add(const void *buffer, int len)
{
    int err;
    err = bloom_add(ppbloom + current, buffer, len);
    if (err == -1)
        return err;

    bloom_count[current]++;

    if (bloom_count[current] >= entries) {
        bloom_count[current] = 0;
        current              = current == PING ? PONG : PING;
        bloom_free(ppbloom + current);
        bloom_init(ppbloom + current, entries, error);
    }

    return 0;
}

void
ppbloom_free()
{
    bloom_free(ppbloom + PING);
    bloom_free(ppbloom + PONG);
}
