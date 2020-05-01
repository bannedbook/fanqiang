/**
 * @file NCDMethodIndex.c
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
#include <limits.h>

#include <misc/hashfun.h>
#include <misc/balloc.h>
#include <misc/strdup.h>

#include "NCDMethodIndex.h"

#include "NCDMethodIndex_hash.h"
#include <structure/CHash_impl.h>

#define GROWARRAY_NAME NamesArray
#define GROWARRAY_OBJECT_TYPE NCDMethodIndex
#define GROWARRAY_ARRAY_MEMBER names
#define GROWARRAY_CAPACITY_MEMBER names_capacity
#define GROWARRAY_MAX_CAPACITY INT_MAX
#include <misc/grow_array.h>

#define GROWARRAY_NAME EntriesArray
#define GROWARRAY_OBJECT_TYPE NCDMethodIndex
#define GROWARRAY_ARRAY_MEMBER entries
#define GROWARRAY_CAPACITY_MEMBER entries_capacity
#define GROWARRAY_MAX_CAPACITY INT_MAX
#include <misc/grow_array.h>

#include <generated/blog_channel_ncd.h>

static int find_method_name (NCDMethodIndex *o, const char *method_name, int *out_entry_idx)
{
    ASSERT(method_name)
    
    NCDMethodIndex__HashRef ref = NCDMethodIndex__Hash_Lookup(&o->hash, o->names, method_name);
    if (ref.link == -1) {
        return 0;
    }
    
    ASSERT(ref.link >= 0)
    ASSERT(ref.link < o->num_names)
    
    struct NCDMethodIndex__method_name *name_entry = ref.ptr;
    ASSERT(!strcmp(name_entry->method_name, method_name))
    ASSERT(name_entry->first_entry >= 0)
    ASSERT(name_entry->first_entry < o->num_entries)
    
    if (out_entry_idx) {
        *out_entry_idx = name_entry->first_entry;
    }
    return 1;
}

static int add_method_name (NCDMethodIndex *o, const char *method_name, int *out_entry_idx)
{
    ASSERT(method_name)
    ASSERT(!find_method_name(o, method_name, NULL))
    
    if (o->num_entries == o->entries_capacity && !EntriesArray_DoubleUp(o)) {
        BLog(BLOG_ERROR, "EntriesArray_DoubleUp failed");
        goto fail0;
    }
    
    if (o->num_names == o->names_capacity && !NamesArray_DoubleUp(o)) {
        BLog(BLOG_ERROR, "NamesArray_DoubleUp failed");
        goto fail0;
    }
    
    struct NCDMethodIndex__entry *entry = &o->entries[o->num_entries];
    entry->obj_type = -1;
    entry->next = -1;
    
    struct NCDMethodIndex__method_name *name_entry = &o->names[o->num_names];
    
    if (!(name_entry->method_name = b_strdup(method_name))) {
        BLog(BLOG_ERROR, "b_strdup failed");
        goto fail0;
    }
    
    name_entry->first_entry = o->num_entries;
    
    NCDMethodIndex__HashRef ref = {name_entry, o->num_names};
    int res = NCDMethodIndex__Hash_Insert(&o->hash, o->names, ref, NULL);
    ASSERT_EXECUTE(res)
    
    o->num_entries++;
    o->num_names++;
    
    if (out_entry_idx) {
        *out_entry_idx = name_entry->first_entry;
    }
    return 1;
    
fail0:
    return 0;
}

int NCDMethodIndex_Init (NCDMethodIndex *o, NCDStringIndex *string_index)
{
    ASSERT(string_index)
    
    o->string_index = string_index;
    
    if (!NamesArray_Init(o, NCDMETHODINDEX_NUM_EXPECTED_METHOD_NAMES)) {
        BLog(BLOG_ERROR, "NamesArray_Init failed");
        goto fail0;
    }
    
    if (!EntriesArray_Init(o, NCDMETHODINDEX_NUM_EXPECTED_ENTRIES)) {
        BLog(BLOG_ERROR, "EntriesArray_Init failed");
        goto fail1;
    }
    
    o->num_names = 0;
    o->num_entries = 0;
    
    if (!NCDMethodIndex__Hash_Init(&o->hash, NCDMETHODINDEX_NUM_EXPECTED_METHOD_NAMES)) {
        BLog(BLOG_ERROR, "NCDMethodIndex__Hash_Init failed");
        goto fail2;
    }
    
    return 1;
    
fail2:
    EntriesArray_Free(o);
fail1:
    NamesArray_Free(o);
fail0:
    return 0;
}

void NCDMethodIndex_Free (NCDMethodIndex *o)
{
    for (int i = 0; i < o->num_names; i++) {
        free(o->names[i].method_name);
    }
    
    NCDMethodIndex__Hash_Free(&o->hash);
    EntriesArray_Free(o);
    NamesArray_Free(o);
}

int NCDMethodIndex_AddMethod (NCDMethodIndex *o, const char *obj_type, size_t obj_type_len, const char *method_name, const struct NCDInterpModule *module)
{
    ASSERT(obj_type)
    ASSERT(method_name)
    ASSERT(module)
    
    NCD_string_id_t obj_type_id = NCDStringIndex_GetBin(o->string_index, obj_type, obj_type_len);
    if (obj_type_id < 0) {
        BLog(BLOG_ERROR, "NCDStringIndex_Get failed");
        goto fail0;
    }
    
    int entry_idx;
    int first_entry_idx;
    
    if (!find_method_name(o, method_name, &first_entry_idx)) {
        if (!add_method_name(o, method_name, &entry_idx)) {
            goto fail0;
        }
        
        ASSERT(entry_idx >= 0)
        ASSERT(entry_idx < o->num_entries)
        
        struct NCDMethodIndex__entry *entry = &o->entries[entry_idx];
        
        entry->obj_type = obj_type_id;
        entry->module = module;
    } else {
        ASSERT(first_entry_idx >= 0)
        ASSERT(first_entry_idx < o->num_entries)
        
        if (o->num_entries == o->entries_capacity && !EntriesArray_DoubleUp(o)) {
            BLog(BLOG_ERROR, "EntriesArray_DoubleUp failed");
            goto fail0;
        }
        
        entry_idx = o->num_entries;
        struct NCDMethodIndex__entry *entry = &o->entries[o->num_entries];
        
        entry->obj_type = obj_type_id;
        entry->module = module;
        
        entry->next = o->entries[first_entry_idx].next;
        o->entries[first_entry_idx].next = o->num_entries;
        
        o->num_entries++;
    }
    
    return entry_idx;
    
fail0:
    return -1;
}

void NCDMethodIndex_RemoveMethod (NCDMethodIndex *o, int method_name_id)
{
    ASSERT(method_name_id >= 0)
    ASSERT(method_name_id < o->num_entries)
    ASSERT(o->entries[method_name_id].obj_type >= 0)
    
    o->entries[method_name_id].obj_type = -1;
}

int NCDMethodIndex_GetMethodNameId (NCDMethodIndex *o, const char *method_name)
{
    ASSERT(method_name)
    
    int first_entry_idx;
    
    if (!find_method_name(o, method_name, &first_entry_idx)) {
        if (!add_method_name(o, method_name, &first_entry_idx)) {
            return -1;
        }
    }
    
    ASSERT(first_entry_idx >= 0)
    ASSERT(first_entry_idx < o->num_entries)
    
    return first_entry_idx;
}

const struct NCDInterpModule * NCDMethodIndex_GetMethodModule (NCDMethodIndex *o, NCD_string_id_t obj_type, int method_name_id)
{
    ASSERT(obj_type >= 0)
    ASSERT(method_name_id >= 0)
    ASSERT(method_name_id < o->num_entries)
    
    do {
        struct NCDMethodIndex__entry *entry = &o->entries[method_name_id];
        
        if (entry->obj_type == obj_type) {
            ASSERT(entry->module)
            return entry->module;
        }
        
        method_name_id = entry->next;
        ASSERT(method_name_id >= -1)
        ASSERT(method_name_id < o->num_entries)
    } while (method_name_id >= 0);
    
    return NULL;
}
