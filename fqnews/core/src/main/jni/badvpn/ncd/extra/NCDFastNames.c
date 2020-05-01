/**
 * @file NCDFastNames.c
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

#include <string.h>

#include <misc/balloc.h>

#include "NCDFastNames.h"

static size_t count_names (const char *str, size_t str_len)
{
    size_t count = 1;
    
    while (str_len > 0) {
        if (*str == '.') {
            count++;
        }
        str++;
        str_len--;
    }
    
    return count;
}

static int add_name (NCDFastNames *o, NCDStringIndex *string_index, const char *str, size_t str_len, const char *remain, size_t remain_len)
{
    ASSERT(str)
    ASSERT(!!o->dynamic_names == (o->num_names > NCD_NUM_FAST_NAMES))
    
    NCD_string_id_t id = NCDStringIndex_GetBin(string_index, str, str_len);
    if (id < 0) {
        return 0;
    }
    
    if (o->num_names < NCD_NUM_FAST_NAMES) {
        o->static_names[o->num_names++] = id;
        return 1;
    }
    
    if (o->num_names == NCD_NUM_FAST_NAMES) {
        size_t num_more = (!remain ? 0 : count_names(remain, remain_len));
        size_t num_all = o->num_names + 1 + num_more;
        
        if (!(o->dynamic_names = BAllocArray(num_all, sizeof(o->dynamic_names[0])))) {
            return 0;
        }
        
        memcpy(o->dynamic_names, o->static_names, NCD_NUM_FAST_NAMES * sizeof(o->dynamic_names[0]));
    }
    
    o->dynamic_names[o->num_names++] = id;
    
    return 1;
}

int NCDFastNames_Init (NCDFastNames *o, NCDStringIndex *string_index, MemRef str)
{
    ASSERT(str.ptr)
    
    o->num_names = 0;
    o->dynamic_names = NULL;
    
    size_t i = 0;
    while (i < str.len) {
        if (str.ptr[i] == '.') {
            if (!add_name(o, string_index, str.ptr, i, str.ptr + (i + 1), str.len - (i + 1))) {
                goto fail;
            }
            str = MemRef_SubFrom(str, i + 1);
            i = 0;
            continue;
        }
        i++;
    }
    
    if (!add_name(o, string_index, str.ptr, i, NULL, 0)) {
        goto fail;
    }
    
    return 1;
    
fail:
    BFree(o->dynamic_names);
    return 0;
}

void NCDFastNames_Free (NCDFastNames *o)
{
    if (o->dynamic_names) {
        BFree(o->dynamic_names);
    }
}

size_t NCDFastNames_GetNumNames (NCDFastNames *o)
{
    ASSERT(o->num_names > 0)
    
    return o->num_names;
}

NCD_string_id_t * NCDFastNames_GetNames (NCDFastNames *o)
{
    return o->dynamic_names ? o->dynamic_names : o->static_names;
}
