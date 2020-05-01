/**
 * @file memref.h
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

#ifndef BADVPN_MEMREF_H
#define BADVPN_MEMREF_H

#include <stddef.h>
#include <string.h>
#include <limits.h>

#include <misc/debug.h>
#include <misc/balloc.h>
#include <misc/strdup.h>

typedef struct {
    char const *ptr;
    size_t len;
} MemRef;

static MemRef MemRef_Make (char const *ptr, size_t len);
static MemRef MemRef_MakeCstr (char const *ptr);
static char MemRef_At (MemRef o, size_t pos);
static void MemRef_AssertRange (MemRef o, size_t offset, size_t length);
static MemRef MemRef_SubFrom (MemRef o, size_t offset);
static MemRef MemRef_SubTo (MemRef o, size_t length);
static MemRef MemRef_Sub (MemRef o, size_t offset, size_t length);
static char * MemRef_StrDup (MemRef o);
static void MemRef_CopyOut (MemRef o, char *out);
static int MemRef_Equal (MemRef o, MemRef other);
static int MemRef_FindChar (MemRef o, char ch, size_t *out_index);

#define MEMREF_LOOP_CHARS__BODY(char_rel_pos_var, char_var, body) \
{ \
    for (size_t char_rel_pos_var = 0; char_rel_pos_var < MemRef__Loop_length; char_rel_pos_var++) { \
        char char_var = MemRef__Loop_o.ptr[MemRef__Loop_offset + char_rel_pos_var]; \
        { body } \
    } \
}

#define MEMREF_LOOP_CHARS_RANGE(o, offset, length, char_rel_pos_var, char_var, body) \
{ \
    MemRef MemRef__Loop_o = (o); \
    size_t MemRef__Loop_offset = (offset); \
    size_t MemRef__Loop_length = (length); \
    MEMREF_LOOP_CHARS__BODY(char_rel_pos_var, char_var, body) \
}

#define MEMREF_LOOP_CHARS(o, char_rel_pos_var, char_var, body) \
{ \
    MemRef MemRef__Loop_o = (o); \
    size_t MemRef__Loop_offset = 0; \
    size_t MemRef__Loop_length = MemRef__Loop_o.len; \
    MEMREF_LOOP_CHARS__BODY(char_rel_pos_var, char_var, body) \
}

//

static MemRef MemRef_Make (char const *ptr, size_t len)
{
    MemRef res;
    res.ptr = ptr;
    res.len = len;
    return res;
}

static MemRef MemRef_MakeCstr (char const *ptr)
{
    ASSERT(ptr)
    
    return MemRef_Make(ptr, strlen(ptr));
}

static char MemRef_At (MemRef o, size_t pos)
{
    ASSERT(o.ptr)
    ASSERT(pos < o.len)
    
    return o.ptr[pos];
}

static void MemRef_AssertRange (MemRef o, size_t offset, size_t length)
{
    ASSERT(offset <= o.len)
    ASSERT(length <= o.len - offset)
}

static MemRef MemRef_SubFrom (MemRef o, size_t offset)
{
    ASSERT(o.ptr)
    ASSERT(offset <= o.len)
    
    return MemRef_Make(o.ptr + offset, o.len - offset);
}

static MemRef MemRef_SubTo (MemRef o, size_t length)
{
    ASSERT(o.ptr)
    ASSERT(length <= o.len)
    
    return MemRef_Make(o.ptr, length);
}

static MemRef MemRef_Sub (MemRef o, size_t offset, size_t length)
{
    ASSERT(o.ptr)
    MemRef_AssertRange(o, offset, length);
    
    return MemRef_Make(o.ptr + offset, length);
}

static char * MemRef_StrDup (MemRef o)
{
    ASSERT(o.ptr)
    
    return b_strdup_bin(o.ptr, o.len);
}

static void MemRef_CopyOut (MemRef o, char *out)
{
    ASSERT(o.ptr)
    ASSERT(out)
    
    memcpy(out, o.ptr, o.len);
}

static int MemRef_Equal (MemRef o, MemRef other)
{
    ASSERT(o.ptr)
    ASSERT(other.ptr)
    
    return (o.len == other.len) && !memcmp(o.ptr, other.ptr, o.len);
}

static int MemRef_FindChar (MemRef o, char ch, size_t *out_index)
{
    ASSERT(o.ptr)
    
    for (size_t i = 0; i < o.len; i++) {
        if (o.ptr[i] == ch) {
            if (out_index) {
                *out_index = i;
            }
            return 1;
        }
    }
    return 0;
}

#endif
