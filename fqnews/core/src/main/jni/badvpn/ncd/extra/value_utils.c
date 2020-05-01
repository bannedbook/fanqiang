/**
 * @file value_utils.c
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

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#include <misc/debug.h>
#include <misc/parse_number.h>
#include <misc/strdup.h>
#include <misc/balloc.h>
#include <system/BTime.h>
#include <ncd/NCDVal.h>
#include <ncd/NCDStringIndex.h>
#include <ncd/NCDModule.h>
#include <ncd/static_strings.h>

#include "value_utils.h"

int ncd_is_none (NCDValRef string)
{
    ASSERT(NCDVal_IsString(string))
    
    if (NCDVal_IsIdString(string)) {
        return NCDVal_IdStringId(string) == NCD_STRING_NONE;
    } else {
        return NCDVal_StringEquals(string, "<none>");
    }
}

NCDValRef ncd_make_boolean (NCDValMem *mem, int value)
{
    ASSERT(mem)
    
    NCD_string_id_t str_id = (value ? NCD_STRING_TRUE : NCD_STRING_FALSE);
    return NCDVal_NewIdString(mem, str_id);
}

int ncd_read_boolean (NCDValRef val, int *out)
{
    ASSERT(out)
    
    if (!NCDVal_IsString(val)) {
        return 0;
    }
    if (NCDVal_IsIdString(val)) {
        *out = NCDVal_IdStringId(val) == NCD_STRING_TRUE;
    } else {
        *out = NCDVal_StringEquals(val, "true");
    }
    return 1;
}

int ncd_read_uintmax (NCDValRef val, uintmax_t *out)
{
    ASSERT(out)
    
    if (!NCDVal_IsString(val)) {
        return 0;
    }
    
    return parse_unsigned_integer(NCDVal_StringMemRef(val), out);
}

int ncd_read_time (NCDValRef val, btime_t *out)
{
    ASSERT(out)
    
    uintmax_t n;
    if (!ncd_read_uintmax(val, &n)) {
        return 0;
    }
    
    if (n > INT64_MAX) {
        return 0;
    }
    
    *out = n;
    return 1;
}

NCD_string_id_t ncd_get_string_id (NCDValRef string)
{
    ASSERT(NCDVal_IsString(string))
    
    if (NCDVal_IsIdString(string)) {
        return NCDVal_IdStringId(string);
    }
    
    return NCDStringIndex_GetBinMr(NCDValMem_StringIndex(string.mem), NCDVal_StringMemRef(string));
}

NCDValRef ncd_make_uintmax (NCDValMem *mem, uintmax_t value)
{
    ASSERT(mem)
    
    int size = compute_decimal_repr_size(value);
    
    NCDValRef val = NCDVal_NewStringUninitialized(mem, size);
    
    if (!NCDVal_IsInvalid(val)) {
        char *data = (char *)NCDVal_StringData(val);
        generate_decimal_repr(value, data, size);
    }
    
    return val;
}

char * ncd_strdup (NCDValRef stringnonulls)
{
    ASSERT(NCDVal_IsStringNoNulls(stringnonulls))
    
    return MemRef_StrDup(NCDVal_StringMemRef(stringnonulls));
}

int ncd_eval_func_args_ext (NCDCall const *call, size_t start, size_t count, NCDValMem *mem, NCDValRef *out)
{
    ASSERT(start <= NCDCall_ArgCount(call))
    ASSERT(count <= NCDCall_ArgCount(call) - start)
    
    *out = NCDVal_NewList(mem, count);
    if (NCDVal_IsInvalid(*out)) {
        goto fail;
    }
    
    for (size_t i = 0; i < count; i++) {
        NCDValRef elem = NCDCall_EvalArg(call, start + i, mem);
        if (NCDVal_IsInvalid(elem)) {
            goto fail;
        }
        if (!NCDVal_ListAppend(*out, elem)) {
            goto fail;
        }
    }
    
    return 1;
    
fail:
    return 0;
}

int ncd_eval_func_args (NCDCall const *call, NCDValMem *mem, NCDValRef *out)
{
    return ncd_eval_func_args_ext(call, 0, NCDCall_ArgCount(call), mem, out);
}
