/**
 * @file cmdline.h
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * @section DESCRIPTION
 * 
 * Command line construction functions.
 */

#ifndef BADVPN_MISC_CMDLINE_H
#define BADVPN_MISC_CMDLINE_H

#include <stddef.h>
#include <stdarg.h>

#include <misc/debug.h>
#include <misc/exparray.h>
#include <misc/strdup.h>
#include <misc/memref.h>

typedef struct {
    struct ExpArray arr;
    size_t n;
} CmdLine;

static int CmdLine_Init (CmdLine *c);
static void CmdLine_Free (CmdLine *c);
static int CmdLine_Append (CmdLine *c, const char *str);
static int CmdLine_AppendNoNull (CmdLine *c, const char *str, size_t str_len);
static int CmdLine_AppendNoNullMr (CmdLine *c, MemRef mr);
static int CmdLine_AppendMulti (CmdLine *c, int num, ...);
static int CmdLine_Finish (CmdLine *c);
static char ** CmdLine_Get (CmdLine *c);

static int _CmdLine_finished (CmdLine *c)
{
    return (c->n > 0 && ((char **)c->arr.v)[c->n - 1] == NULL);
}

int CmdLine_Init (CmdLine *c)
{
    if (!ExpArray_init(&c->arr, sizeof(char *), 16)) {
        return 0;
    }
    
    c->n = 0;
    
    return 1;
}

void CmdLine_Free (CmdLine *c)
{
    for (size_t i = 0; i < c->n; i++) {
        free(((char **)c->arr.v)[i]);
    }
    
    free(c->arr.v);
}

int CmdLine_Append (CmdLine *c, const char *str)
{
    ASSERT(str)
    ASSERT(!_CmdLine_finished(c))
    
    if (!ExpArray_resize(&c->arr, c->n + 1)) {
        return 0;
    }
    
    if (!(((char **)c->arr.v)[c->n] = strdup(str))) {
        return 0;
    }
    
    c->n++;
    
    return 1;
}

int CmdLine_AppendNoNull (CmdLine *c, const char *str, size_t str_len)
{
    ASSERT(str)
    ASSERT(!memchr(str, '\0', str_len))
    ASSERT(!_CmdLine_finished(c))
    
    if (!ExpArray_resize(&c->arr, c->n + 1)) {
        return 0;
    }
    
    if (!(((char **)c->arr.v)[c->n] = b_strdup_bin(str, str_len))) {
        return 0;
    }
    
    c->n++;
    
    return 1;
}

int CmdLine_AppendNoNullMr (CmdLine *c, MemRef mr)
{
    return CmdLine_AppendNoNull(c, mr.ptr, mr.len);
}

int CmdLine_AppendMulti (CmdLine *c, int num, ...)
{
    int res = 1;
    
    va_list vl;
    va_start(vl, num);
    
    for (int i = 0; i < num; i++) {
        const char *str = va_arg(vl, const char *);
        if (!CmdLine_Append(c, str)) {
            res = 0;
            break;
        }
    }
    
    va_end(vl);
    
    return res;
}

int CmdLine_Finish (CmdLine *c)
{
    ASSERT(!_CmdLine_finished(c))
    
    if (!ExpArray_resize(&c->arr, c->n + 1)) {
        return 0;
    }
    
    ((char **)c->arr.v)[c->n] = NULL;
    
    c->n++;
    
    return 1;
}

char ** CmdLine_Get (CmdLine *c)
{
    ASSERT(_CmdLine_finished(c))
    
    return (char **)c->arr.v;
}

#endif
