/**
 * @file make_name_indices.h
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

#ifndef BADVPN_MAKE_NAME_INDICES_H
#define BADVPN_MAKE_NAME_INDICES_H

#include <stdlib.h>
#include <stddef.h>

#include <misc/strdup.h>
#include <misc/balloc.h>
#include <misc/debug.h>
#include <ncd/NCDStringIndex.h>

static int ncd_make_name_indices (NCDStringIndex *string_index, const char *name, NCD_string_id_t **out_varnames, size_t *out_num_names) WARN_UNUSED;

static size_t split_string_inplace2__indices (char *str, char del)
{
    ASSERT(str)
    
    size_t num_extra_parts = 0;
    
    while (*str) {
        if (*str == del) {
            *str = '\0';
            num_extra_parts++;
        }
        str++;
    }
    
    return num_extra_parts;
}

static int ncd_make_name_indices (NCDStringIndex *string_index, const char *name, NCD_string_id_t **out_varnames, size_t *out_num_names)
{
    ASSERT(string_index)
    ASSERT(name)
    ASSERT(out_varnames)
    ASSERT(out_num_names)
    
    char *data = b_strdup(name);
    if (!data) {
        goto fail0;
    }
    
    size_t num_names = split_string_inplace2__indices(data, '.') + 1;
    
    NCD_string_id_t *varnames = BAllocArray(num_names, sizeof(varnames[0]));
    if (!varnames) {
        goto fail1;
    }
    
    char *cur = data;
    for (size_t i = 0; i < num_names; i++) {
        NCD_string_id_t id = NCDStringIndex_Get(string_index, cur);
        if (id < 0) {
            goto fail2;
        }
        
        varnames[i] = id;
        cur += strlen(cur) + 1;
    }
    
    free(data);
    
    *out_varnames = varnames;
    *out_num_names = num_names;
    return 1;
    
fail2:
    BFree(varnames);
fail1:
    free(data);
fail0:
    return 0;
}

#endif
