/**
 * @file NCDStringIndex.c
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
#include <stdlib.h>

#include <misc/hashfun.h>
#include <misc/strdup.h>
#include <misc/array_length.h>
#include <base/BLog.h>

#include "NCDStringIndex.h"

#include "NCDStringIndex_hash.h"
#include <structure/CHash_impl.h>

#define GROWARRAY_NAME Array
#define GROWARRAY_OBJECT_TYPE NCDStringIndex
#define GROWARRAY_ARRAY_MEMBER entries
#define GROWARRAY_CAPACITY_MEMBER entries_capacity
#define GROWARRAY_MAX_CAPACITY NCD_STRING_ID_MAX
#include <misc/grow_array.h>

#include <generated/blog_channel_ncd.h>

// NOTE: keep synchronized with static_strings.h
static const char *static_strings[] = {
    "",
    "_args",
    "_arg0",
    "_arg1",
    "_arg2",
    "_arg3",
    "_arg4",
    "_arg5",
    "_arg6",
    "_arg7",
    "_arg8",
    "_arg9",
    "_arg10",
    "_arg11",
    "_arg12",
    "_arg13",
    "_arg14",
    "_arg15",
    "_arg16",
    "_arg17",
    "_arg18",
    "_arg19",
    "true",
    "false",
    "<none>",
    "_caller",
    "succeeded",
    "is_error",
    "not_eof",
    "length",
    "type",
    "exit_status",
    "size",
    "eof",
    "_scope",
};

static NCD_string_id_t do_get (NCDStringIndex *o, const char *str, size_t str_len)
{
    ASSERT(str)
    
    NCDStringIndex_hash_key key = {str, str_len};
    NCDStringIndex__HashRef ref = NCDStringIndex__Hash_Lookup(&o->hash, o->entries, key);
    ASSERT(ref.link == -1 || ref.link >= 0)
    ASSERT(ref.link == -1 || ref.link < o->entries_size)
    ASSERT(ref.link == -1 || (ref.ptr->str_len == str_len && !memcmp(ref.ptr->str, str, str_len)))
    
    if (ref.link != -1) {
        return ref.link;
    }
    
    if (o->entries_size == o->entries_capacity) {
        if (!Array_DoubleUp(o)) {
            BLog(BLOG_ERROR, "Array_DoubleUp failed");
            return -1;
        }
        
        if (!NCDStringIndex__Hash_MultiplyBuckets(&o->hash, o->entries, 1)) {
            BLog(BLOG_ERROR, "NCDStringIndex__Hash_MultiplyBuckets failed");
            return -1;
        }
    }
    
    ASSERT(o->entries_size < o->entries_capacity)
    
    struct NCDStringIndex__entry *entry = &o->entries[o->entries_size];
    
    if (!(entry->str = b_strdup_bin(str, str_len))) {
        BLog(BLOG_ERROR, "b_strdup_bin failed");
        return -1;
    }
    entry->str_len = str_len;
    entry->has_nulls = !!memchr(str, '\0', str_len);
    
    NCDStringIndex__HashRef newref = {entry, o->entries_size};
    int res = NCDStringIndex__Hash_Insert(&o->hash, o->entries, newref, NULL);
    ASSERT_EXECUTE(res)
    
    return o->entries_size++;
}

int NCDStringIndex_Init (NCDStringIndex *o)
{
    o->entries_size = 0;
    
    if (!Array_Init(o, NCDSTRINGINDEX_INITIAL_CAPACITY)) {
        BLog(BLOG_ERROR, "Array_Init failed");
        goto fail0;
    }
    
    if (!NCDStringIndex__Hash_Init(&o->hash, NCDSTRINGINDEX_INITIAL_HASH_BUCKETS)) {
        BLog(BLOG_ERROR, "NCDStringIndex__Hash_Init failed");
        goto fail1;
    }
    
    for (size_t i = 0; i < B_ARRAY_LENGTH(static_strings); i++) {
        if (do_get(o, static_strings[i], strlen(static_strings[i])) < 0) {
            goto fail2;
        }
    }
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail2:
    for (NCD_string_id_t i = 0; i < o->entries_size; i++) {
        free(o->entries[i].str);
    }
    NCDStringIndex__Hash_Free(&o->hash);
fail1:
    Array_Free(o);
fail0:
    return 0;
}

void NCDStringIndex_Free (NCDStringIndex *o)
{
    DebugObject_Free(&o->d_obj);
    
    for (NCD_string_id_t i = 0; i < o->entries_size; i++) {
        free(o->entries[i].str);
    }
    
    NCDStringIndex__Hash_Free(&o->hash);
    Array_Free(o);
}

NCD_string_id_t NCDStringIndex_Lookup (NCDStringIndex *o, const char *str)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(str)
    
    return NCDStringIndex_LookupBin(o, str, strlen(str));
}

NCD_string_id_t NCDStringIndex_LookupBin (NCDStringIndex *o, const char *str, size_t str_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(str)
    
    NCDStringIndex_hash_key key = {str, str_len};
    NCDStringIndex__HashRef ref = NCDStringIndex__Hash_Lookup(&o->hash, o->entries, key);
    ASSERT(ref.link == -1 || ref.link >= 0)
    ASSERT(ref.link == -1 || ref.link < o->entries_size)
    ASSERT(ref.link == -1 || (ref.ptr->str_len == str_len && !memcmp(ref.ptr->str, str, str_len)))
    
    return ref.link;
}

NCD_string_id_t NCDStringIndex_Get (NCDStringIndex *o, const char *str)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(str)
    
    return NCDStringIndex_GetBin(o, str, strlen(str));
}

NCD_string_id_t NCDStringIndex_GetBin (NCDStringIndex *o, const char *str, size_t str_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(str)
    
    return do_get(o, str, str_len);
}

NCD_string_id_t NCDStringIndex_GetBinMr (NCDStringIndex *o, MemRef str)
{
    return NCDStringIndex_GetBin(o, str.ptr, str.len);
}

MemRef NCDStringIndex_Value (NCDStringIndex *o, NCD_string_id_t id)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(id >= 0)
    ASSERT(id < o->entries_size)
    ASSERT(o->entries[id].str)
    
    return MemRef_Make(o->entries[id].str, o->entries[id].str_len);
}

int NCDStringIndex_HasNulls (NCDStringIndex *o, NCD_string_id_t id)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(id >= 0)
    ASSERT(id < o->entries_size)
    ASSERT(o->entries[id].str)
    
    return o->entries[id].has_nulls;
}

int NCDStringIndex_GetRequests (NCDStringIndex *o, struct NCD_string_request *requests)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(requests)
    
    while (requests->str) {
        NCD_string_id_t id = NCDStringIndex_Get(o, requests->str);
        if (id < 0) {
            return 0;
        }
        requests->id = id;
        requests++;
    }
    
    return 1;
}
