/**
 * @file bstring.h
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
 */

#ifndef BADVPN_BSTRING_H
#define BADVPN_BSTRING_H

#include <misc/debug.h>

#include <stdlib.h>
#include <string.h>

#define BSTRING_TYPE_STATIC 5
#define BSTRING_TYPE_DYNAMIC 7
#define BSTRING_TYPE_EXTERNAL 11

#define BSTRING_STATIC_SIZE 23
#define BSTRING_STATIC_MAX (BSTRING_STATIC_SIZE - 1)

typedef struct {
    union {
        struct {
            char type;
            char static_string[BSTRING_STATIC_SIZE];
        } us;
        struct {
            char type;
            char *dynamic_string;
        } ud;
        struct {
            char type;
            const char *external_string;
        } ue;
    } u;
} BString;

static void BString__assert (BString *o)
{
    switch (o->u.us.type) {
        case BSTRING_TYPE_STATIC:
        case BSTRING_TYPE_DYNAMIC:
        case BSTRING_TYPE_EXTERNAL:
            return;
    }
    
    ASSERT(0);
}

static int BString_Init (BString *o, const char *str)
{
    if (strlen(str) <= BSTRING_STATIC_MAX) {
        strcpy(o->u.us.static_string, str);
        o->u.us.type = BSTRING_TYPE_STATIC;
    } else {
        if (!(o->u.ud.dynamic_string = malloc(strlen(str) + 1))) {
            return 0;
        }
        strcpy(o->u.ud.dynamic_string, str);
        o->u.ud.type = BSTRING_TYPE_DYNAMIC;
    }
    
    BString__assert(o);
    return 1;
}

static void BString_InitStatic (BString *o, const char *str)
{
    ASSERT(strlen(str) <= BSTRING_STATIC_MAX)
    
    strcpy(o->u.us.static_string, str);
    o->u.us.type = BSTRING_TYPE_STATIC;
    
    BString__assert(o);
}

static void BString_InitExternal (BString *o, const char *str)
{
    o->u.ue.external_string = str;
    o->u.ue.type = BSTRING_TYPE_EXTERNAL;
    
    BString__assert(o);
}

static void BString_InitAllocated (BString *o, char *str)
{
    o->u.ud.dynamic_string = str;
    o->u.ud.type = BSTRING_TYPE_DYNAMIC;
    
    BString__assert(o);
}

static void BString_Free (BString *o)
{
    BString__assert(o);
    
    if (o->u.ud.type == BSTRING_TYPE_DYNAMIC) {
        free(o->u.ud.dynamic_string);
    }
}

static const char * BString_Get (BString *o)
{
    BString__assert(o);
    
    switch (o->u.us.type) {
        case BSTRING_TYPE_STATIC: return o->u.us.static_string;
        case BSTRING_TYPE_DYNAMIC: return o->u.ud.dynamic_string;
        case BSTRING_TYPE_EXTERNAL: return o->u.ue.external_string;
    }
    
    ASSERT(0);
    return NULL;
}

#endif
