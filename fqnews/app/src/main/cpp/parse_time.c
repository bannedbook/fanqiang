/*
Copyright (c) 2003-2006 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "polipo.h"

const time_t time_t_max = ((time_t)~(1U << 31));

static inline int
d2i(char c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    else
        return -1;
}

static int
parse_int(const char *buf, int i, int len, int *val_return)
{
    int val, d;

    if(i >= len)
        return -1;

    val = d2i(buf[i]);
    if(val < 0)
        return -1;
    else
        i++;

    while(i < len) {
        d = d2i(buf[i]);
        if(d < 0)
            break;
        val = val * 10 + d;
        i++;
    }
    *val_return = val;
    return i;
}

static const char month_names[12][3] = {
    "jan", "feb", "mar", "apr", "may", "jun",
    "jul", "aug", "sep", "oct", "nov", "dec",
};

static int
skip_word(const char *buf, int i, int len)
{
    if(i >= len)
        return -1;

    if(!letter(buf[i]))
       return -1;

    while(i < len) {
        if(!letter(buf[i]))
            break;
        i++;
    }

    return i;
}

static int
parse_month(const char *buf, int i, int len, int *val_return)
{
    int j, k, l;
    j = skip_word(buf, i, len);
    if(j != i + 3)
        return -1;
    for(k = 0; k < 12; k++) {
        for(l = 0; l < 3; l++) {
            if(lwr(buf[i + l]) != month_names[k][l])
                break;
        }
        if(l == 3)
            break;
    }
    if(k >= 12)
        return -1;
    *val_return = k;
    return j;
}

static int
issep(char c)
{
    return c == ' ' || c == '\t' || c == ',' || c == ':' || c == '-';
}

int
skip_separator(const char *buf, int i, int len)
{
    if(i >= len)
        return -1;

    if(issep(buf[i]))
        i++;
    else
        return -1;

    while(i < len) {
        if(issep(buf[i]))
            i++;
        else
            break;
    }
    return i;
}

int
parse_time(const char *buf, int offset, int len, time_t *time_return)
{
    struct tm tm;
    time_t t;
    int i = offset;

    i = skip_word(buf, i, len); if(i < 0) return -1;
    i = skip_separator(buf, i, len); if(i < 0) return -1;

    if(i >= len)
        return -1;

    if(d2i(buf[i]) >= 0) {
        i = parse_int(buf, i, len, &tm.tm_mday); if(i < 0) return -1;
        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = parse_month(buf, i, len, &tm.tm_mon); if(i < 0) return -1;
        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = parse_int(buf, i, len, &tm.tm_year); if(i < 0) return -1;
        if(tm.tm_year < 100)
            tm.tm_year += 1900;
        if(tm.tm_year < 1937)
            tm.tm_year += 100;
        if(tm.tm_year < 1937)
            return -1;

        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = parse_int(buf, i, len, &tm.tm_hour); if(i < 0) return -1;
        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = parse_int(buf, i, len, &tm.tm_min); if(i < 0) return -1;
        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = parse_int(buf, i, len, &tm.tm_sec); if(i < 0) return -1;
        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = skip_word(buf, i, len); if(i < 0) return -1;
    } else {                    /* funny American format */
        i = parse_month(buf, i, len, &tm.tm_mon); if(i < 0) return -1;
        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = parse_int(buf, i, len, &tm.tm_mday); if(i < 0) return -1;
        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = parse_int(buf, i, len, &tm.tm_hour); if(i < 0) return -1;
        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = parse_int(buf, i, len, &tm.tm_min); if(i < 0) return -1;
        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = parse_int(buf, i, len, &tm.tm_sec); if(i < 0) return -1;
        i = skip_separator(buf, i, len); if(i < 0) return -1;
        i = parse_int(buf, i, len, &tm.tm_year); if(i < 0) return -1;
        if(tm.tm_year < 100)
            tm.tm_year += 1900;
        if(tm.tm_year < 1937)
            tm.tm_year += 100;
        if(tm.tm_year < 1937 || tm.tm_year > 2040)
            return -1;
    }

    if(tm.tm_year < 2038) {
        tm.tm_year -= 1900;
        tm.tm_isdst = -1;
        t = mktime_gmt(&tm);
        if(t == -1)
            return -1;
    } else {
        t = time_t_max;
    }

    *time_return = t;
    return i;
}

int
format_time(char *buf, int i, int len, time_t t)
{
    struct tm *tm;
    int rc;

    if(i < 0 || i > len)
        return -1;

    tm = gmtime(&t);
    if(tm == NULL)
        return -1;
    rc = strftime(buf + i, len - i, "%a, %d %b %Y %H:%M:%S GMT", tm);
    if(rc <= 0)                 /* yes, that's <= */
        return -1;
    return i + rc;
}
