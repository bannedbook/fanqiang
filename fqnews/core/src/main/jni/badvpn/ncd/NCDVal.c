/**
 * @file NCDVal.c
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
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>

#include <misc/balloc.h>
#include <misc/strdup.h>
#include <misc/offset.h>
#include <structure/CAvl.h>
#include <base/BLog.h>

#include "NCDVal.h"

#include <generated/blog_channel_NCDVal.h>

#define NCDVAL_FIRST_SIZE 256
#define NCDVAL_MAX_DEPTH 32

#define TYPE_MASK_EXTERNAL_TYPE ((1 << 3) - 1)
#define TYPE_MASK_INTERNAL_TYPE ((1 << 5) - 1)
#define TYPE_SHIFT_DEPTH 5

#define STOREDSTRING_TYPE (NCDVAL_STRING | (0 << 3))
#define IDSTRING_TYPE (NCDVAL_STRING | (1 << 3))
#define EXTERNALSTRING_TYPE (NCDVAL_STRING | (2 << 3))

#define NCDVAL_INSTR_PLACEHOLDER 0
#define NCDVAL_INSTR_REINSERT 1
#define NCDVAL_INSTR_BUMPDEPTH 2

struct NCDVal__ref {
    NCDVal__idx next;
    BRefTarget *target;
};

struct NCDVal__string {
    int type;
    NCDVal__idx length;
    char data[];
};

struct NCDVal__list {
    int type;
    NCDVal__idx maxcount;
    NCDVal__idx count;
    NCDVal__idx elem_indices[];
};

struct NCDVal__mapelem {
    NCDVal__idx key_idx;
    NCDVal__idx val_idx;
    NCDVal__idx tree_child[2];
    NCDVal__idx tree_parent;
    int8_t tree_balance;
};

struct NCDVal__idstring {
    int type;
    NCD_string_id_t string_id;
};

struct NCDVal__externalstring {
    int type;
    const char *data;
    size_t length;
    struct NCDVal__ref ref;
};

typedef struct NCDVal__mapelem NCDVal__maptree_entry;
typedef NCDValMem *NCDVal__maptree_arg;

#include "NCDVal_maptree.h"
#include <structure/CAvl_decl.h>

struct NCDVal__map {
    int type;
    NCDVal__idx maxcount;
    NCDVal__idx count;
    NCDVal__MapTree tree;
    struct NCDVal__mapelem elems[];
};

struct NCDVal__instr {
    int type;
    union {
        struct {
            NCDVal__idx plid;
            NCDVal__idx plidx;
        } placeholder;
        struct {
            NCDVal__idx mapidx;
            NCDVal__idx elempos;
        } reinsert;
        struct {
            NCDVal__idx parent_idx;
            NCDVal__idx child_idx_idx;
        } bumpdepth;
    };
};

static int make_type (int internal_type, int depth)
{
    ASSERT(internal_type == NCDVAL_LIST ||
           internal_type == NCDVAL_MAP ||
           internal_type == STOREDSTRING_TYPE ||
           internal_type == IDSTRING_TYPE ||
           internal_type == EXTERNALSTRING_TYPE)
    ASSERT(depth >= 0)
    ASSERT(depth <= NCDVAL_MAX_DEPTH)
    
    return (internal_type | (depth << TYPE_SHIFT_DEPTH));
}

static int get_external_type (int type)
{
    return (type & TYPE_MASK_EXTERNAL_TYPE);
}

static int get_internal_type (int type)
{
    return (type & TYPE_MASK_INTERNAL_TYPE);
}

static int get_depth (int type)
{
    return (type >> TYPE_SHIFT_DEPTH);
}

static int bump_depth (int *type_ptr, int elem_depth)
{
    if (get_depth(*type_ptr) < elem_depth + 1) {
        if (elem_depth + 1 > NCDVAL_MAX_DEPTH) {
            return 0;
        }
        *type_ptr = make_type(get_internal_type(*type_ptr), elem_depth + 1);
    }
    
    return 1;
}

static void * buffer_at (NCDValMem *o, NCDVal__idx idx)
{
    ASSERT(idx >= 0)
    ASSERT(idx < o->used)
    
    return ((o->size == NCDVAL_FASTBUF_SIZE) ? o->fastbuf : o->allocd_buf) + idx;
}

static NCDVal__idx buffer_allocate (NCDValMem *o, NCDVal__idx alloc_size, NCDVal__idx align)
{
    NCDVal__idx mod = o->used % align;
    NCDVal__idx align_extra = mod ? (align - mod) : 0;
    
    if (alloc_size > NCDVAL_MAXIDX - align_extra) {
        return -1;
    }
    NCDVal__idx aligned_alloc_size = align_extra + alloc_size;
    
    if (aligned_alloc_size > o->size - o->used) {
        NCDVal__idx newsize = (o->size == NCDVAL_FASTBUF_SIZE) ? NCDVAL_FIRST_SIZE : o->size;
        while (aligned_alloc_size > newsize - o->used) {
            if (newsize > NCDVAL_MAXIDX / 2) {
                return -1;
            }
            newsize *= 2;
        }
        
        char *newbuf;
        
        if (o->size == NCDVAL_FASTBUF_SIZE) {
            newbuf = malloc(newsize);
            if (!newbuf) {
                return -1;
            }
            memcpy(newbuf, o->fastbuf, o->used);
        } else {
            newbuf = realloc(o->allocd_buf, newsize);
            if (!newbuf) {
                return -1;
            }
        }
        
        o->size = newsize;
        o->allocd_buf = newbuf;
    }
    
    NCDVal__idx idx = o->used + align_extra;
    o->used += aligned_alloc_size;
    
    return idx;
}

static NCDValRef make_ref (NCDValMem *mem, NCDVal__idx idx)
{
    ASSERT(idx == -1 || mem)
    
    NCDValRef ref = {mem, idx};
    return ref;
}

static void assert_mem (NCDValMem *mem)
{
    ASSERT(mem)
    ASSERT(mem->string_index)
    ASSERT(mem->size == NCDVAL_FASTBUF_SIZE || mem->size >= NCDVAL_FIRST_SIZE)
    ASSERT(mem->used >= 0)
    ASSERT(mem->used <= mem->size)
}

static void assert_external (NCDValMem *mem, const void *e_buf, size_t e_len)
{
#ifndef NDEBUG
    const char *e_cbuf = e_buf;
    char *buf = (mem->size == NCDVAL_FASTBUF_SIZE) ? mem->fastbuf : mem->allocd_buf;
    ASSERT(e_cbuf >= buf + mem->size || e_cbuf + e_len <= buf)
#endif
}

static void assert_val_only (NCDValMem *mem, NCDVal__idx idx)
{
    // placeholders
    if (idx < -1) {
        return;
    }
    
    ASSERT(idx >= 0)
    ASSERT(idx + sizeof(int) <= mem->used)
    
#ifndef NDEBUG
    int *type_ptr = buffer_at(mem, idx);
    
    ASSERT(get_depth(*type_ptr) >= 0)
    ASSERT(get_depth(*type_ptr) <= NCDVAL_MAX_DEPTH)
    
    switch (get_internal_type(*type_ptr)) {
        case STOREDSTRING_TYPE: {
            ASSERT(idx + sizeof(struct NCDVal__string) <= mem->used)
            struct NCDVal__string *str_e = buffer_at(mem, idx);
            ASSERT(str_e->length >= 0)
            ASSERT(idx + sizeof(struct NCDVal__string) + str_e->length + 1 <= mem->used)
        } break;
        case NCDVAL_LIST: {
            ASSERT(idx + sizeof(struct NCDVal__list) <= mem->used)
            struct NCDVal__list *list_e = buffer_at(mem, idx);
            ASSERT(list_e->maxcount >= 0)
            ASSERT(list_e->count >= 0)
            ASSERT(list_e->count <= list_e->maxcount)
            ASSERT(idx + sizeof(struct NCDVal__list) + list_e->maxcount * sizeof(NCDVal__idx) <= mem->used)
        } break;
        case NCDVAL_MAP: {
            ASSERT(idx + sizeof(struct NCDVal__map) <= mem->used)
            struct NCDVal__map *map_e = buffer_at(mem, idx);
            ASSERT(map_e->maxcount >= 0)
            ASSERT(map_e->count >= 0)
            ASSERT(map_e->count <= map_e->maxcount)
            ASSERT(idx + sizeof(struct NCDVal__map) + map_e->maxcount * sizeof(struct NCDVal__mapelem) <= mem->used)
        } break;
        case IDSTRING_TYPE: {
            ASSERT(idx + sizeof(struct NCDVal__idstring) <= mem->used)
            struct NCDVal__idstring *ids_e = buffer_at(mem, idx);
            ASSERT(ids_e->string_id >= 0)
        } break;
        case EXTERNALSTRING_TYPE: {
            ASSERT(idx + sizeof(struct NCDVal__externalstring) <= mem->used)
            struct NCDVal__externalstring *exs_e = buffer_at(mem, idx);
            ASSERT(exs_e->data)
            ASSERT(!exs_e->ref.target || exs_e->ref.next >= -1)
            ASSERT(!exs_e->ref.target || exs_e->ref.next < mem->used)
        } break;
        default: ASSERT(0);
    }
#endif
}

static void assert_val (NCDValRef val)
{
    assert_mem(val.mem);
    assert_val_only(val.mem, val.idx);
}

static NCDValMapElem make_map_elem (NCDVal__idx elemidx)
{
    ASSERT(elemidx >= 0 || elemidx == -1)
    
    NCDValMapElem me = {elemidx};
    return me;
}

static void assert_map_elem_only (NCDValRef map, NCDVal__idx elemidx)
{
#ifndef NDEBUG
    struct NCDVal__map *map_e = buffer_at(map.mem, map.idx);
    ASSERT(elemidx >= map.idx + offsetof(struct NCDVal__map, elems))
    ASSERT(elemidx < map.idx + offsetof(struct NCDVal__map, elems) + map_e->count * sizeof(struct NCDVal__mapelem))

    struct NCDVal__mapelem *me_e = buffer_at(map.mem, elemidx);
    assert_val_only(map.mem, me_e->key_idx);
    assert_val_only(map.mem, me_e->val_idx);
#endif
}

static void assert_map_elem (NCDValRef map, NCDValMapElem me)
{
    ASSERT(NCDVal_IsMap(map))
    assert_map_elem_only(map, me.elemidx);
}

static NCDVal__idx make_map_elem_idx (NCDVal__idx mapidx, NCDVal__idx pos)
{
    return mapidx + offsetof(struct NCDVal__map, elems) + pos * sizeof(struct NCDVal__mapelem);
}

static int get_val_depth (NCDValRef val)
{
    ASSERT(val.idx != -1)
    
    // handle placeholders
    if (val.idx < 0) {
        return 0;
    }
    
    int *elem_type_ptr = buffer_at(val.mem, val.idx);
    int depth = get_depth(*elem_type_ptr);
    ASSERT(depth >= 0)
    ASSERT(depth <= NCDVAL_MAX_DEPTH)
    
    return depth;
}

static void register_ref (NCDValMem *o, NCDVal__idx refidx, struct NCDVal__ref *ref)
{
    ASSERT(ref == buffer_at(o, refidx))
    ASSERT(ref->target)
    
    ref->next = o->first_ref;
    o->first_ref = refidx;
}

#include "NCDVal_maptree.h"
#include <structure/CAvl_impl.h>

void NCDValMem_Init (NCDValMem *o, NCDStringIndex *string_index)
{
    ASSERT(string_index)
    
    o->string_index = string_index;
    o->size = NCDVAL_FASTBUF_SIZE;
    o->used = 0;
    o->first_ref = -1;
}

void NCDValMem_Free (NCDValMem *o)
{
    assert_mem(o);
    
    NCDVal__idx refidx = o->first_ref;
    while (refidx != -1) {
        struct NCDVal__ref *ref = buffer_at(o, refidx);
        ASSERT(ref->target)
        BRefTarget_Deref(ref->target);
        refidx = ref->next;
    }
    
    if (o->size != NCDVAL_FASTBUF_SIZE) {
        BFree(o->allocd_buf);
    }
}

int NCDValMem_InitCopy (NCDValMem *o, NCDValMem *other)
{
    assert_mem(other);
    
    o->string_index = other->string_index;
    o->size = other->size;
    o->used = other->used;
    o->first_ref = other->first_ref;
    
    if (other->size == NCDVAL_FASTBUF_SIZE) {
        memcpy(o->fastbuf, other->fastbuf, other->used);
    } else {
        o->allocd_buf = BAlloc(other->size);
        if (!o->allocd_buf) {
            goto fail0;
        }
        memcpy(o->allocd_buf, other->allocd_buf, other->used);
    }
    
    NCDVal__idx refidx = o->first_ref;
    while (refidx != -1) {
        struct NCDVal__ref *ref = buffer_at(o, refidx);
        ASSERT(ref->target)
        if (!BRefTarget_Ref(ref->target)) {
            goto fail1;
        }
        refidx = ref->next;
    }
    
    return 1;
    
fail1:;
    NCDVal__idx undo_refidx = o->first_ref;
    while (undo_refidx != refidx) {
        struct NCDVal__ref *ref = buffer_at(o, undo_refidx);
        BRefTarget_Deref(ref->target);
        undo_refidx = ref->next;
    }
    if (other->size != NCDVAL_FASTBUF_SIZE) {
        BFree(o->allocd_buf);
    }
fail0:
    return 0;
}

NCDStringIndex * NCDValMem_StringIndex (NCDValMem *o)
{
    assert_mem(o);
    
    return o->string_index;
}

void NCDVal_Assert (NCDValRef val)
{
    ASSERT(val.idx == -1 || (assert_val(val), 1))
}

int NCDVal_IsInvalid (NCDValRef val)
{
    NCDVal_Assert(val);
    
    return (val.idx == -1);
}

int NCDVal_IsPlaceholder (NCDValRef val)
{
    NCDVal_Assert(val);
    
    return (val.idx < -1);
}

int NCDVal_Type (NCDValRef val)
{
    assert_val(val);
    
    if (val.idx < -1) {
        return NCDVAL_PLACEHOLDER;
    }
    
    int *type_ptr = buffer_at(val.mem, val.idx);
    
    return get_external_type(*type_ptr);
}

NCDValRef NCDVal_NewInvalid (void)
{
    NCDValRef ref = {NULL, -1};
    return ref;
}

NCDValRef NCDVal_NewPlaceholder (NCDValMem *mem, int plid)
{
    assert_mem(mem);
    ASSERT(plid >= 0)
    ASSERT(plid < NCDVAL_TOPPLID)
    
    NCDValRef ref = {mem, NCDVAL_MINIDX + plid};
    return ref;
}

int NCDVal_PlaceholderId (NCDValRef val)
{
    ASSERT(NCDVal_IsPlaceholder(val))
    
    return (val.idx - NCDVAL_MINIDX);
}

NCDValRef NCDVal_NewCopy (NCDValMem *mem, NCDValRef val)
{
    assert_mem(mem);
    assert_val(val);
    
    if (val.idx < -1) {
        return NCDVal_NewPlaceholder(mem, NCDVal_PlaceholderId(val));
    }
    
    void *ptr = buffer_at(val.mem, val.idx);
    
    switch (get_internal_type(*(int *)ptr)) {
        case STOREDSTRING_TYPE: {
            struct NCDVal__string *str_e = ptr;
            
            NCDVal__idx size = sizeof(struct NCDVal__string) + str_e->length + 1;
            NCDVal__idx idx = buffer_allocate(mem, size, __alignof(struct NCDVal__string));
            if (idx < 0) {
                goto fail;
            }
            
            str_e = buffer_at(val.mem, val.idx);
            struct NCDVal__string *new_str_e = buffer_at(mem, idx);
            
            memcpy(new_str_e, str_e, size);
            
            return make_ref(mem, idx);
        } break;
        
        case NCDVAL_LIST: {
            struct NCDVal__list *list_e = ptr;
            
            NCDVal__idx size = sizeof(struct NCDVal__list) + list_e->maxcount * sizeof(NCDVal__idx);
            NCDVal__idx idx = buffer_allocate(mem, size, __alignof(struct NCDVal__list));
            if (idx < 0) {
                goto fail;
            }
            
            list_e = buffer_at(val.mem, val.idx);
            struct NCDVal__list *new_list_e = buffer_at(mem, idx);
            
            *new_list_e = *list_e;
            
            NCDVal__idx count = list_e->count;
            
            for (NCDVal__idx i = 0; i < count; i++) {
                NCDValRef elem_copy = NCDVal_NewCopy(mem, make_ref(val.mem, list_e->elem_indices[i]));
                if (NCDVal_IsInvalid(elem_copy)) {
                    goto fail;
                }
                
                list_e = buffer_at(val.mem, val.idx);
                new_list_e = buffer_at(mem, idx);
                
                new_list_e->elem_indices[i] = elem_copy.idx;
            }
            
            return make_ref(mem, idx);
        } break;
        
        case NCDVAL_MAP: {
            size_t count = NCDVal_MapCount(val);
            
            NCDValRef copy = NCDVal_NewMap(mem, count);
            if (NCDVal_IsInvalid(copy)) {
                goto fail;
            }
            
            for (NCDValMapElem e = NCDVal_MapFirst(val); !NCDVal_MapElemInvalid(e); e = NCDVal_MapNext(val, e)) {
                NCDValRef key_copy = NCDVal_NewCopy(mem, NCDVal_MapElemKey(val, e));
                NCDValRef val_copy = NCDVal_NewCopy(mem, NCDVal_MapElemVal(val, e));
                if (NCDVal_IsInvalid(key_copy) || NCDVal_IsInvalid(val_copy)) {
                    goto fail;
                }
                
                int inserted;
                if (!NCDVal_MapInsert(copy, key_copy, val_copy, &inserted)) {
                    goto fail;
                }
                ASSERT_EXECUTE(inserted)
            }
            
            return copy;
        } break;
        
        case IDSTRING_TYPE: {
            NCDVal__idx size = sizeof(struct NCDVal__idstring);
            NCDVal__idx idx = buffer_allocate(mem, size, __alignof(struct NCDVal__idstring));
            if (idx < 0) {
                goto fail;
            }
            
            struct NCDVal__idstring *ids_e = buffer_at(val.mem, val.idx);
            struct NCDVal__idstring *new_ids_e = buffer_at(mem, idx);
            
            *new_ids_e = *ids_e;
            
            return make_ref(mem, idx);
        } break;
        
        case EXTERNALSTRING_TYPE: {
            struct NCDVal__externalstring *exs_e = ptr;
            
            return NCDVal_NewExternalString(mem, exs_e->data, exs_e->length, exs_e->ref.target);
        } break;
        
        default: ASSERT(0);
    }
    
    ASSERT(0);
    
fail:
    return NCDVal_NewInvalid();
}

int NCDVal_Compare (NCDValRef val1, NCDValRef val2)
{
    assert_val(val1);
    assert_val(val2);
    
    int type1 = NCDVal_Type(val1);
    int type2 = NCDVal_Type(val2);
    
    if (type1 != type2) {
        return (type1 > type2) - (type1 < type2);
    }
    
    switch (type1) {
        case NCDVAL_STRING: {
            size_t len1 = NCDVal_StringLength(val1);
            size_t len2 = NCDVal_StringLength(val2);
            size_t min_len = len1 < len2 ? len1 : len2;
            
            int cmp = NCDVal_StringMemCmp(val1, val2, 0, 0, min_len);
            if (cmp) {
                return (cmp > 0) - (cmp < 0);
            }
            
            return (len1 > len2) - (len1 < len2);
        } break;
        
        case NCDVAL_LIST: {
            size_t count1 = NCDVal_ListCount(val1);
            size_t count2 = NCDVal_ListCount(val2);
            size_t min_count = count1 < count2 ? count1 : count2;
            
            for (size_t i = 0; i < min_count; i++) {
                NCDValRef ev1 = NCDVal_ListGet(val1, i);
                NCDValRef ev2 = NCDVal_ListGet(val2, i);
                
                int cmp = NCDVal_Compare(ev1, ev2);
                if (cmp) {
                    return cmp;
                }
            }
            
            return (count1 > count2) - (count1 < count2);
        } break;
        
        case NCDVAL_MAP: {
            NCDValMapElem e1 = NCDVal_MapOrderedFirst(val1);
            NCDValMapElem e2 = NCDVal_MapOrderedFirst(val2);
            
            while (1) {
                int inv1 = NCDVal_MapElemInvalid(e1);
                int inv2 = NCDVal_MapElemInvalid(e2);
                if (inv1 || inv2) {
                    return inv2 - inv1;
                }
                
                NCDValRef key1 = NCDVal_MapElemKey(val1, e1);
                NCDValRef key2 = NCDVal_MapElemKey(val2, e2);
                
                int cmp = NCDVal_Compare(key1, key2);
                if (cmp) {
                    return cmp;
                }
                
                NCDValRef value1 = NCDVal_MapElemVal(val1, e1);
                NCDValRef value2 = NCDVal_MapElemVal(val2, e2);
                
                cmp = NCDVal_Compare(value1, value2);
                if (cmp) {
                    return cmp;
                }
                
                e1 = NCDVal_MapOrderedNext(val1, e1);
                e2 = NCDVal_MapOrderedNext(val2, e2);
            }
        } break;
        
        case NCDVAL_PLACEHOLDER: {
            int plid1 = NCDVal_PlaceholderId(val1);
            int plid2 = NCDVal_PlaceholderId(val2);
            
            return (plid1 > plid2) - (plid1 < plid2);
        } break;
        
        default:
            ASSERT(0);
            return 0;
    }
}

NCDValSafeRef NCDVal_ToSafe (NCDValRef val)
{
    NCDVal_Assert(val);
    
    NCDValSafeRef sval = {val.idx};
    return sval;
}

NCDValRef NCDVal_FromSafe (NCDValMem *mem, NCDValSafeRef sval)
{
    assert_mem(mem);
    ASSERT(sval.idx == -1 || (assert_val_only(mem, sval.idx), 1))
    
    NCDValRef val = {mem, sval.idx};
    return val;
}

NCDValRef NCDVal_Moved (NCDValMem *mem, NCDValRef val)
{
    assert_mem(mem);
    ASSERT(val.idx == -1 || (assert_val_only(mem, val.idx), 1))
    
    NCDValRef val2 = {mem, val.idx};
    return val2;
}

int NCDVal_IsSafeRefPlaceholder (NCDValSafeRef sval)
{
    return (sval.idx < -1);
}

int NCDVal_GetSafeRefPlaceholderId (NCDValSafeRef sval)
{
    ASSERT(NCDVal_IsSafeRefPlaceholder(sval))
    
    return (sval.idx - NCDVAL_MINIDX);
}

int NCDVal_IsString (NCDValRef val)
{
    assert_val(val);
    
    return NCDVal_Type(val) == NCDVAL_STRING;
}

int NCDVal_IsStoredString (NCDValRef val)
{
    assert_val(val);
    
    return !(val.idx < -1) && get_internal_type(*(int *)buffer_at(val.mem, val.idx)) == STOREDSTRING_TYPE;
}

int NCDVal_IsIdString (NCDValRef val)
{
    assert_val(val);
    
    return !(val.idx < -1) && get_internal_type(*(int *)buffer_at(val.mem, val.idx)) == IDSTRING_TYPE;
}

int NCDVal_IsExternalString (NCDValRef val)
{
    assert_val(val);
    
    return !(val.idx < -1) && get_internal_type(*(int *)buffer_at(val.mem, val.idx)) == EXTERNALSTRING_TYPE;
}

int NCDVal_IsStringNoNulls (NCDValRef val)
{
    assert_val(val);
    
    return NCDVal_Type(val) == NCDVAL_STRING && !NCDVal_StringHasNulls(val);
}

NCDValRef NCDVal_NewString (NCDValMem *mem, const char *data)
{
    assert_mem(mem);
    ASSERT(data)
    assert_external(mem, data, strlen(data));
    
    return NCDVal_NewStringBin(mem, (const uint8_t *)data, strlen(data));
}

NCDValRef NCDVal_NewStringBin (NCDValMem *mem, const uint8_t *data, size_t len)
{
    assert_mem(mem);
    ASSERT(len == 0 || data)
    assert_external(mem, data, len);
    
    if (len > NCDVAL_MAXIDX - sizeof(struct NCDVal__string) - 1) {
        goto fail;
    }
    
    NCDVal__idx size = sizeof(struct NCDVal__string) + len + 1;
    NCDVal__idx idx = buffer_allocate(mem, size, __alignof(struct NCDVal__string));
    if (idx < 0) {
        goto fail;
    }
    
    struct NCDVal__string *str_e = buffer_at(mem, idx);
    str_e->type = make_type(STOREDSTRING_TYPE, 0);
    str_e->length = len;
    if (len > 0) {
        memcpy(str_e->data, data, len);
    }
    str_e->data[len] = '\0';
    
    return make_ref(mem, idx);
    
fail:
    return NCDVal_NewInvalid();
}

NCDValRef NCDVal_NewStringBinMr (NCDValMem *mem, MemRef data)
{
    return NCDVal_NewStringBin(mem, (uint8_t const *)data.ptr, data.len);
}

NCDValRef NCDVal_NewStringUninitialized (NCDValMem *mem, size_t len)
{
    assert_mem(mem);
    
    if (len > NCDVAL_MAXIDX - sizeof(struct NCDVal__string) - 1) {
        goto fail;
    }
    
    NCDVal__idx size = sizeof(struct NCDVal__string) + len + 1;
    NCDVal__idx idx = buffer_allocate(mem, size, __alignof(struct NCDVal__string));
    if (idx < 0) {
        goto fail;
    }
    
    struct NCDVal__string *str_e = buffer_at(mem, idx);
    str_e->type = make_type(STOREDSTRING_TYPE, 0);
    str_e->length = len;
    str_e->data[len] = '\0';
    
    return make_ref(mem, idx);
    
fail:
    return NCDVal_NewInvalid();
}

NCDValRef NCDVal_NewIdString (NCDValMem *mem, NCD_string_id_t string_id)
{
    assert_mem(mem);
    ASSERT(string_id >= 0)
    
    NCDVal__idx size = sizeof(struct NCDVal__idstring);
    NCDVal__idx idx = buffer_allocate(mem, size, __alignof(struct NCDVal__idstring));
    if (idx < 0) {
        goto fail;
    }
    
    struct NCDVal__idstring *ids_e = buffer_at(mem, idx);
    ids_e->type = make_type(IDSTRING_TYPE, 0);
    ids_e->string_id = string_id;
    
    return make_ref(mem, idx);
    
fail:
    return NCDVal_NewInvalid();
}

NCDValRef NCDVal_NewExternalString (NCDValMem *mem, const char *data, size_t len,
                                    BRefTarget *ref_target)
{
    assert_mem(mem);
    ASSERT(data)
    assert_external(mem, data, len);
    
    NCDVal__idx size = sizeof(struct NCDVal__externalstring);
    NCDVal__idx idx = buffer_allocate(mem, size, __alignof(struct NCDVal__externalstring));
    if (idx < 0) {
        goto fail;
    }
    
    if (ref_target) {
        if (!BRefTarget_Ref(ref_target)) {
            goto fail;
        }
    }
    
    struct NCDVal__externalstring *exs_e = buffer_at(mem, idx);
    exs_e->type = make_type(EXTERNALSTRING_TYPE, 0);
    exs_e->data = data;
    exs_e->length = len;
    exs_e->ref.target = ref_target;
    
    if (ref_target) {
        register_ref(mem, idx + offsetof(struct NCDVal__externalstring, ref), &exs_e->ref);
    }
    
    return make_ref(mem, idx);
    
fail:
    return NCDVal_NewInvalid();
}

const char * NCDVal_StringData (NCDValRef string)
{
    ASSERT(NCDVal_IsString(string))
    
    void *ptr = buffer_at(string.mem, string.idx);
    
    switch (get_internal_type(*(int *)ptr)) {
        case STOREDSTRING_TYPE: {
            struct NCDVal__string *str_e = ptr;
            return str_e->data;
        } break;
        
        case IDSTRING_TYPE: {
            struct NCDVal__idstring *ids_e = ptr;
            const char *value = NCDStringIndex_Value(string.mem->string_index, ids_e->string_id).ptr;
            return value;
        } break;
        
        case EXTERNALSTRING_TYPE: {
            struct NCDVal__externalstring *exs_e = ptr;
            return exs_e->data;
        } break;
        
        default:
            ASSERT(0);
            return NULL;
    }
}

size_t NCDVal_StringLength (NCDValRef string)
{
    ASSERT(NCDVal_IsString(string))
    
    void *ptr = buffer_at(string.mem, string.idx);
    
    switch (get_internal_type(*(int *)ptr)) {
        case STOREDSTRING_TYPE: {
            struct NCDVal__string *str_e = ptr;
            return str_e->length;
        } break;
        
        case IDSTRING_TYPE: {
            struct NCDVal__idstring *ids_e = ptr;
            return NCDStringIndex_Value(string.mem->string_index, ids_e->string_id).len;
        } break;
        
        case EXTERNALSTRING_TYPE: {
            struct NCDVal__externalstring *exs_e = ptr;
            return exs_e->length;
        } break;
        
        default:
            ASSERT(0);
            return 0;
    }
}

MemRef NCDVal_StringMemRef (NCDValRef string)
{
    ASSERT(NCDVal_IsString(string))
    
    void *ptr = buffer_at(string.mem, string.idx);
    
    switch (get_internal_type(*(int *)ptr)) {
        case STOREDSTRING_TYPE: {
            struct NCDVal__string *str_e = ptr;
            return MemRef_Make(str_e->data, str_e->length);
        } break;
        
        case IDSTRING_TYPE: {
            struct NCDVal__idstring *ids_e = ptr;
            return NCDStringIndex_Value(string.mem->string_index, ids_e->string_id);
        } break;
        
        case EXTERNALSTRING_TYPE: {
            struct NCDVal__externalstring *exs_e = ptr;
            return MemRef_Make(exs_e->data, exs_e->length);
        } break;
        
        default: {
            ASSERT(0);
            return MemRef_Make(NULL, 0);
        } break;
    }
}

int NCDVal_StringNullTerminate (NCDValRef string, NCDValNullTermString *out)
{
    ASSERT(NCDVal_IsString(string))
    ASSERT(out)
    
    void *ptr = buffer_at(string.mem, string.idx);
    
    switch (get_internal_type(*(int *)ptr)) {
        case STOREDSTRING_TYPE: {
            struct NCDVal__string *str_e = ptr;
            out->data = str_e->data;
            out->is_allocated = 0;
            return 1;
        } break;
        
        case IDSTRING_TYPE: {
            struct NCDVal__idstring *ids_e = ptr;
            out->data = (char *)NCDStringIndex_Value(string.mem->string_index, ids_e->string_id).ptr;
            out->is_allocated = 0;
            return 1;
        } break;
        
        case EXTERNALSTRING_TYPE: {
            struct NCDVal__externalstring *exs_e = ptr;
            
            char *copy = b_strdup_bin(exs_e->data, exs_e->length);
            if (!copy) {
                return 0;
            }
            
            out->data = copy;
            out->is_allocated = 1;
            return 1;
        } break;
        
        default:
            ASSERT(0);
            return 0;
    }
}

NCDValNullTermString NCDValNullTermString_NewDummy (void)
{
    NCDValNullTermString nts;
    nts.data = NULL;
    nts.is_allocated = 0;
    return nts;
}

void NCDValNullTermString_Free (NCDValNullTermString *o)
{
    if (o->is_allocated) {
        BFree(o->data);
    }
}

NCD_string_id_t NCDVal_IdStringId (NCDValRef idstring)
{
    ASSERT(NCDVal_IsIdString(idstring))
    
    struct NCDVal__idstring *ids_e = buffer_at(idstring.mem, idstring.idx);
    return ids_e->string_id;
}

BRefTarget * NCDVal_ExternalStringTarget (NCDValRef externalstring)
{
    ASSERT(NCDVal_IsExternalString(externalstring))
    
    struct NCDVal__externalstring *exs_e = buffer_at(externalstring.mem, externalstring.idx);
    return exs_e->ref.target;
}

int NCDVal_StringHasNulls (NCDValRef string)
{
    ASSERT(NCDVal_IsString(string))
    
    void *ptr = buffer_at(string.mem, string.idx);
    
    switch (get_internal_type(*(int *)ptr)) {
        case IDSTRING_TYPE: {
            struct NCDVal__idstring *ids_e = ptr;
            return NCDStringIndex_HasNulls(string.mem->string_index, ids_e->string_id);
        } break;
        
        case STOREDSTRING_TYPE:
        case EXTERNALSTRING_TYPE: {
            return MemRef_FindChar(NCDVal_StringMemRef(string), '\0', NULL);
        } break;
        
        default:
            ASSERT(0);
            return 0;
    }
}

int NCDVal_StringEquals (NCDValRef string, const char *data)
{
    ASSERT(NCDVal_IsString(string))
    ASSERT(data)
    
    size_t data_len = strlen(data);
    
    return NCDVal_StringLength(string) == data_len && NCDVal_StringRegionEquals(string, 0, data_len, data);
}

int NCDVal_StringEqualsId (NCDValRef string, NCD_string_id_t string_id)
{
    ASSERT(NCDVal_IsString(string))
    ASSERT(string_id >= 0)
    
    void *ptr = buffer_at(string.mem, string.idx);
    
    switch (get_internal_type(*(int *)ptr)) {
        case STOREDSTRING_TYPE: {
            struct NCDVal__string *str_e = ptr;
            return MemRef_Equal(NCDStringIndex_Value(string.mem->string_index, string_id), MemRef_Make(str_e->data, str_e->length));
        } break;
        
        case IDSTRING_TYPE: {
            struct NCDVal__idstring *ids_e = ptr;
            return ids_e->string_id == string_id;
        } break;
        
        case EXTERNALSTRING_TYPE: {
            struct NCDVal__externalstring *exs_e = ptr;
            return MemRef_Equal(NCDStringIndex_Value(string.mem->string_index, string_id), MemRef_Make(exs_e->data, exs_e->length));
        } break;
        
        default:
            ASSERT(0);
            return 0;
    }
}

int NCDVal_StringMemCmp (NCDValRef string1, NCDValRef string2, size_t start1, size_t start2, size_t length)
{
    ASSERT(NCDVal_IsString(string1))
    ASSERT(NCDVal_IsString(string2))
    ASSERT(start1 <= NCDVal_StringLength(string1))
    ASSERT(start2 <= NCDVal_StringLength(string2))
    ASSERT(length <= NCDVal_StringLength(string1) - start1)
    ASSERT(length <= NCDVal_StringLength(string2) - start2)
    
    return memcmp(NCDVal_StringData(string1) + start1, NCDVal_StringData(string2) + start2, length);
}

void NCDVal_StringCopyOut (NCDValRef string, size_t start, size_t length, char *dst)
{
    ASSERT(NCDVal_IsString(string))
    ASSERT(start <= NCDVal_StringLength(string))
    ASSERT(length <= NCDVal_StringLength(string) - start)
    
    memcpy(dst, NCDVal_StringData(string) + start, length);
}

int NCDVal_StringRegionEquals (NCDValRef string, size_t start, size_t length, const char *data)
{
    ASSERT(NCDVal_IsString(string))
    ASSERT(start <= NCDVal_StringLength(string))
    ASSERT(length <= NCDVal_StringLength(string) - start)
    
    return !memcmp(NCDVal_StringData(string) + start, data, length);
}

int NCDVal_IsList (NCDValRef val)
{
    assert_val(val);
    
    return NCDVal_Type(val) == NCDVAL_LIST;
}

NCDValRef NCDVal_NewList (NCDValMem *mem, size_t maxcount)
{
    assert_mem(mem);
    
    if (maxcount > (NCDVAL_MAXIDX - sizeof(struct NCDVal__list)) / sizeof(NCDVal__idx)) {
        goto fail;
    }
    
    NCDVal__idx size = sizeof(struct NCDVal__list) + maxcount * sizeof(NCDVal__idx);
    NCDVal__idx idx = buffer_allocate(mem, size, __alignof(struct NCDVal__list));
    if (idx < 0) {
        goto fail;
    }
    
    struct NCDVal__list *list_e = buffer_at(mem, idx);
    list_e->type = make_type(NCDVAL_LIST, 0);
    list_e->maxcount = maxcount;
    list_e->count = 0;
    
    return make_ref(mem, idx);
    
fail:
    return NCDVal_NewInvalid();
}

int NCDVal_ListAppend (NCDValRef list, NCDValRef elem)
{
    ASSERT(NCDVal_IsList(list))
    ASSERT(NCDVal_ListCount(list) < NCDVal_ListMaxCount(list))
    ASSERT(elem.mem == list.mem)
    assert_val_only(list.mem, elem.idx);
    
    struct NCDVal__list *list_e = buffer_at(list.mem, list.idx);
    
    int new_type = list_e->type;
    if (!bump_depth(&new_type, get_val_depth(elem))) {
        return 0;
    }
    
    list_e->type = new_type;
    list_e->elem_indices[list_e->count++] = elem.idx;
    
    return 1;
}

size_t NCDVal_ListCount (NCDValRef list)
{
    ASSERT(NCDVal_IsList(list))
    
    struct NCDVal__list *list_e = buffer_at(list.mem, list.idx);
    
    return list_e->count;
}

size_t NCDVal_ListMaxCount (NCDValRef list)
{
    ASSERT(NCDVal_IsList(list))
    
    struct NCDVal__list *list_e = buffer_at(list.mem, list.idx);
    
    return list_e->maxcount;
}

NCDValRef NCDVal_ListGet (NCDValRef list, size_t pos)
{
    ASSERT(NCDVal_IsList(list))
    ASSERT(pos < NCDVal_ListCount(list))
    
    struct NCDVal__list *list_e = buffer_at(list.mem, list.idx);
    
    ASSERT(pos < list_e->count)
    assert_val_only(list.mem, list_e->elem_indices[pos]);
    
    return make_ref(list.mem, list_e->elem_indices[pos]);
}

int NCDVal_ListRead (NCDValRef list, int num, ...)
{
    ASSERT(NCDVal_IsList(list))
    ASSERT(num >= 0)
    
    struct NCDVal__list *list_e = buffer_at(list.mem, list.idx);
    
    if (num != list_e->count) {
        return 0;
    }
    
    va_list ap;
    va_start(ap, num);
    
    for (int i = 0; i < num; i++) {
        NCDValRef *dest = va_arg(ap, NCDValRef *);
        *dest = make_ref(list.mem, list_e->elem_indices[i]);
    }
    
    va_end(ap);
    
    return 1;
}

int NCDVal_ListReadStart (NCDValRef list, int start, int num, ...)
{
    ASSERT(NCDVal_IsList(list))
    ASSERT(start <= NCDVal_ListCount(list))
    ASSERT(num >= 0)
    
    struct NCDVal__list *list_e = buffer_at(list.mem, list.idx);
    
    if (num != list_e->count - start) {
        return 0;
    }
    
    va_list ap;
    va_start(ap, num);
    
    for (int i = 0; i < num; i++) {
        NCDValRef *dest = va_arg(ap, NCDValRef *);
        *dest = make_ref(list.mem, list_e->elem_indices[start + i]);
    }
    
    va_end(ap);
    
    return 1;
}

int NCDVal_ListReadHead (NCDValRef list, int num, ...)
{
    ASSERT(NCDVal_IsList(list))
    ASSERT(num >= 0)
    
    struct NCDVal__list *list_e = buffer_at(list.mem, list.idx);
    
    if (num > list_e->count) {
        return 0;
    }
    
    va_list ap;
    va_start(ap, num);
    
    for (int i = 0; i < num; i++) {
        NCDValRef *dest = va_arg(ap, NCDValRef *);
        *dest = make_ref(list.mem, list_e->elem_indices[i]);
    }
    
    va_end(ap);
    
    return 1;
}

int NCDVal_IsMap (NCDValRef val)
{
    assert_val(val);
    
    return NCDVal_Type(val) == NCDVAL_MAP;
}

NCDValRef NCDVal_NewMap (NCDValMem *mem, size_t maxcount)
{
    assert_mem(mem);
    
    if (maxcount > (NCDVAL_MAXIDX - sizeof(struct NCDVal__map)) / sizeof(struct NCDVal__mapelem)) {
        goto fail;
    }
    
    NCDVal__idx size = sizeof(struct NCDVal__map) + maxcount * sizeof(struct NCDVal__mapelem);
    NCDVal__idx idx = buffer_allocate(mem, size, __alignof(struct NCDVal__map));
    if (idx < 0) {
        goto fail;
    }
    
    struct NCDVal__map *map_e = buffer_at(mem, idx);
    map_e->type = make_type(NCDVAL_MAP, 0);
    map_e->maxcount = maxcount;
    map_e->count = 0;
    NCDVal__MapTree_Init(&map_e->tree);
    
    return make_ref(mem, idx);
    
fail:
    return NCDVal_NewInvalid();
}

int NCDVal_MapInsert (NCDValRef map, NCDValRef key, NCDValRef val, int *out_inserted)
{
    ASSERT(NCDVal_IsMap(map))
    ASSERT(NCDVal_MapCount(map) < NCDVal_MapMaxCount(map))
    ASSERT(key.mem == map.mem)
    ASSERT(val.mem == map.mem)
    assert_val_only(map.mem, key.idx);
    assert_val_only(map.mem, val.idx);
    
    struct NCDVal__map *map_e = buffer_at(map.mem, map.idx);
    
    int new_type = map_e->type;
    if (!bump_depth(&new_type, get_val_depth(key)) || !bump_depth(&new_type, get_val_depth(val))) {
        goto fail0;
    }
    
    NCDVal__idx elemidx = make_map_elem_idx(map.idx, map_e->count);
    
    struct NCDVal__mapelem *me_e = buffer_at(map.mem, elemidx);
    ASSERT(me_e == &map_e->elems[map_e->count])
    me_e->key_idx = key.idx;
    me_e->val_idx = val.idx;
    
    int res = NCDVal__MapTree_Insert(&map_e->tree, map.mem, NCDVal__MapTreeDeref(map.mem, elemidx), NULL);
    if (!res) {
        if (out_inserted) {
            *out_inserted = 0;
        }
        return 1;
    }
    
    map_e->type = new_type;
    map_e->count++;
    
    if (out_inserted) {
        *out_inserted = 1;
    }
    return 1;
    
fail0:
    return 0;
}

size_t NCDVal_MapCount (NCDValRef map)
{
    ASSERT(NCDVal_IsMap(map))
    
    struct NCDVal__map *map_e = buffer_at(map.mem, map.idx);
    
    return map_e->count;
}

size_t NCDVal_MapMaxCount (NCDValRef map)
{
    ASSERT(NCDVal_IsMap(map))
    
    struct NCDVal__map *map_e = buffer_at(map.mem, map.idx);
    
    return map_e->maxcount;
}

int NCDVal_MapElemInvalid (NCDValMapElem me)
{
    ASSERT(me.elemidx >= 0 || me.elemidx == -1)
    
    return me.elemidx < 0;
}

NCDValMapElem NCDVal_MapFirst (NCDValRef map)
{
    ASSERT(NCDVal_IsMap(map))
    
    struct NCDVal__map *map_e = buffer_at(map.mem, map.idx);
    
    if (map_e->count == 0) {
        return make_map_elem(-1);
    }
    
    NCDVal__idx elemidx = make_map_elem_idx(map.idx, 0);
    assert_map_elem_only(map, elemidx);
    
    return make_map_elem(elemidx);
}

NCDValMapElem NCDVal_MapNext (NCDValRef map, NCDValMapElem me)
{
    assert_map_elem(map, me);
    
    struct NCDVal__map *map_e = buffer_at(map.mem, map.idx);
    ASSERT(map_e->count > 0)
    
    NCDVal__idx last_elemidx = make_map_elem_idx(map.idx, map_e->count - 1);
    ASSERT(me.elemidx <= last_elemidx)
    
    if (me.elemidx == last_elemidx) {
        return make_map_elem(-1);
    }
    
    NCDVal__idx elemidx = me.elemidx + sizeof(struct NCDVal__mapelem);
    assert_map_elem_only(map, elemidx);
    
    return make_map_elem(elemidx);
}

NCDValMapElem NCDVal_MapOrderedFirst (NCDValRef map)
{
    ASSERT(NCDVal_IsMap(map))
    
    struct NCDVal__map *map_e = buffer_at(map.mem, map.idx);
    
    NCDVal__MapTreeRef ref = NCDVal__MapTree_GetFirst(&map_e->tree, map.mem);
    ASSERT(ref.link == -1 || (assert_map_elem_only(map, ref.link), 1))
    
    return make_map_elem(ref.link);
}

NCDValMapElem NCDVal_MapOrderedNext (NCDValRef map, NCDValMapElem me)
{
    assert_map_elem(map, me);
    
    struct NCDVal__map *map_e = buffer_at(map.mem, map.idx);
    
    NCDVal__MapTreeRef ref = NCDVal__MapTree_GetNext(&map_e->tree, map.mem, NCDVal__MapTreeDeref(map.mem, me.elemidx));
    ASSERT(ref.link == -1 || (assert_map_elem_only(map, ref.link), 1))
    
    return make_map_elem(ref.link);
}

NCDValRef NCDVal_MapElemKey (NCDValRef map, NCDValMapElem me)
{
    assert_map_elem(map, me);
    
    struct NCDVal__mapelem *me_e = buffer_at(map.mem, me.elemidx);
    
    return make_ref(map.mem, me_e->key_idx);
}

NCDValRef NCDVal_MapElemVal (NCDValRef map, NCDValMapElem me)
{
    assert_map_elem(map, me);
    
    struct NCDVal__mapelem *me_e = buffer_at(map.mem, me.elemidx);
    
    return make_ref(map.mem, me_e->val_idx);
}

NCDValMapElem NCDVal_MapFindKey (NCDValRef map, NCDValRef key)
{
    ASSERT(NCDVal_IsMap(map))
    assert_val(key);
    
    struct NCDVal__map *map_e = buffer_at(map.mem, map.idx);
    
    NCDVal__MapTreeRef ref = NCDVal__MapTree_LookupExact(&map_e->tree, map.mem, key);
    ASSERT(ref.link == -1 || (assert_map_elem_only(map, ref.link), 1))
    
    return make_map_elem(ref.link);
}

NCDValRef NCDVal_MapGetValue (NCDValRef map, const char *key_str)
{
    ASSERT(NCDVal_IsMap(map))
    ASSERT(key_str)
    
    NCDValMem mem;
    mem.string_index = map.mem->string_index;
    mem.size = NCDVAL_FASTBUF_SIZE;
    mem.used = sizeof(struct NCDVal__externalstring);
    mem.first_ref = -1;
    
    struct NCDVal__externalstring *exs_e = (void *)mem.fastbuf;
    exs_e->type = make_type(EXTERNALSTRING_TYPE, 0);
    exs_e->data = key_str;
    exs_e->length = strlen(key_str);
    exs_e->ref.target = NULL;
    
    NCDValRef key = make_ref(&mem, 0);
    
    NCDValMapElem elem = NCDVal_MapFindKey(map, key);
    if (NCDVal_MapElemInvalid(elem)) {
        return NCDVal_NewInvalid();
    }
    
    return NCDVal_MapElemVal(map, elem);
}

static void replaceprog_build_recurser (NCDValMem *mem, NCDVal__idx idx, size_t *out_num_instr, NCDValReplaceProg *prog)
{
    ASSERT(idx >= 0)
    assert_val_only(mem, idx);
    ASSERT(out_num_instr)
    
    *out_num_instr = 0;
    
    void *ptr = buffer_at(mem, idx);
    
    struct NCDVal__instr instr;
    
    switch (get_internal_type(*((int *)(ptr)))) {
        case STOREDSTRING_TYPE:
        case IDSTRING_TYPE:
        case EXTERNALSTRING_TYPE: {
        } break;
        
        case NCDVAL_LIST: {
            struct NCDVal__list *list_e = ptr;
            
            for (NCDVal__idx i = 0; i < list_e->count; i++) {
                int elem_changed = 0;
                
                if (list_e->elem_indices[i] < -1) {
                    if (prog) {
                        instr.type = NCDVAL_INSTR_PLACEHOLDER;
                        instr.placeholder.plid = list_e->elem_indices[i] - NCDVAL_MINIDX;
                        instr.placeholder.plidx = idx + offsetof(struct NCDVal__list, elem_indices) + i * sizeof(NCDVal__idx);
                        prog->instrs[prog->num_instrs++] = instr;
                    }
                    (*out_num_instr)++;
                    elem_changed = 1;
                } else {
                    size_t elem_num_instr;
                    replaceprog_build_recurser(mem, list_e->elem_indices[i], &elem_num_instr, prog);
                    (*out_num_instr) += elem_num_instr;
                    if (elem_num_instr > 0) {
                        elem_changed = 1;
                    }
                }
                
                if (elem_changed) {
                    if (prog) {
                        instr.type = NCDVAL_INSTR_BUMPDEPTH;
                        instr.bumpdepth.parent_idx = idx;
                        instr.bumpdepth.child_idx_idx = idx + offsetof(struct NCDVal__list, elem_indices) + i * sizeof(NCDVal__idx);
                        prog->instrs[prog->num_instrs++] = instr;
                    }
                    (*out_num_instr)++;
                }
            }
        } break;
        
        case NCDVAL_MAP: {
            struct NCDVal__map *map_e = ptr;
            
            for (NCDVal__idx i = 0; i < map_e->count; i++) {
                int key_changed = 0;
                int val_changed = 0;
                
                if (map_e->elems[i].key_idx < -1) {
                    if (prog) {
                        instr.type = NCDVAL_INSTR_PLACEHOLDER;
                        instr.placeholder.plid = map_e->elems[i].key_idx - NCDVAL_MINIDX;
                        instr.placeholder.plidx = idx + offsetof(struct NCDVal__map, elems) + i * sizeof(struct NCDVal__mapelem) + offsetof(struct NCDVal__mapelem, key_idx);
                        prog->instrs[prog->num_instrs++] = instr;
                    }
                    (*out_num_instr)++;
                    key_changed = 1;
                } else {
                    size_t key_num_instr;
                    replaceprog_build_recurser(mem, map_e->elems[i].key_idx, &key_num_instr, prog);
                    (*out_num_instr) += key_num_instr;
                    if (key_num_instr > 0) {
                        key_changed = 1;
                    }
                }
                
                if (map_e->elems[i].val_idx < -1) {
                    if (prog) {
                        instr.type = NCDVAL_INSTR_PLACEHOLDER;
                        instr.placeholder.plid = map_e->elems[i].val_idx - NCDVAL_MINIDX;
                        instr.placeholder.plidx = idx + offsetof(struct NCDVal__map, elems) + i * sizeof(struct NCDVal__mapelem) + offsetof(struct NCDVal__mapelem, val_idx);
                        prog->instrs[prog->num_instrs++] = instr;
                    }
                    (*out_num_instr)++;
                    val_changed = 1;
                } else {
                    size_t val_num_instr;
                    replaceprog_build_recurser(mem, map_e->elems[i].val_idx, &val_num_instr, prog);
                    (*out_num_instr) += val_num_instr;
                    if (val_num_instr > 0) {
                        val_changed = 1;
                    }
                }
                
                if (key_changed) {
                    if (prog) {
                        instr.type = NCDVAL_INSTR_REINSERT;
                        instr.reinsert.mapidx = idx;
                        instr.reinsert.elempos = i;
                        prog->instrs[prog->num_instrs++] = instr;
                    }
                    (*out_num_instr)++;
                    
                    if (prog) {
                        instr.type = NCDVAL_INSTR_BUMPDEPTH;
                        instr.bumpdepth.parent_idx = idx;
                        instr.bumpdepth.child_idx_idx = idx + offsetof(struct NCDVal__map, elems) + i * sizeof(struct NCDVal__mapelem) + offsetof(struct NCDVal__mapelem, key_idx);
                        prog->instrs[prog->num_instrs++] = instr;
                    }
                    (*out_num_instr)++;
                }
                
                if (val_changed) {
                    if (prog) {
                        instr.type = NCDVAL_INSTR_BUMPDEPTH;
                        instr.bumpdepth.parent_idx = idx;
                        instr.bumpdepth.child_idx_idx = idx + offsetof(struct NCDVal__map, elems) + i * sizeof(struct NCDVal__mapelem) + offsetof(struct NCDVal__mapelem, val_idx);
                        prog->instrs[prog->num_instrs++] = instr;
                    }
                    (*out_num_instr)++;
                }
            }
        } break;
        
        default: ASSERT(0);
    }
}

int NCDValReplaceProg_Init (NCDValReplaceProg *o, NCDValRef val)
{
    assert_val(val);
    ASSERT(!NCDVal_IsPlaceholder(val))
    
    size_t num_instrs;
    replaceprog_build_recurser(val.mem, val.idx, &num_instrs, NULL);
    
    if (!(o->instrs = BAllocArray(num_instrs, sizeof(o->instrs[0])))) {
        BLog(BLOG_ERROR, "BAllocArray failed");
        return 0;
    }
    
    o->num_instrs = 0;
    
    size_t num_instrs2;
    replaceprog_build_recurser(val.mem, val.idx, &num_instrs2, o);
    
    ASSERT(num_instrs2 == num_instrs)
    ASSERT(o->num_instrs == num_instrs)
    
    return 1;
}

void NCDValReplaceProg_Free (NCDValReplaceProg *o)
{
    BFree(o->instrs);
}

int NCDValReplaceProg_Execute (NCDValReplaceProg prog, NCDValMem *mem, NCDVal_replace_func replace, void *arg)
{
    assert_mem(mem);
    ASSERT(replace)
    
    for (size_t i = 0; i < prog.num_instrs; i++) {
        struct NCDVal__instr instr = prog.instrs[i];
        
        switch (instr.type) {
            case NCDVAL_INSTR_PLACEHOLDER: {
#ifndef NDEBUG
                NCDVal__idx *check_plptr = buffer_at(mem, instr.placeholder.plidx);
                ASSERT(*check_plptr < -1)
                ASSERT(*check_plptr - NCDVAL_MINIDX == instr.placeholder.plid)
#endif
                NCDValRef repval;
                if (!replace(arg, instr.placeholder.plid, mem, &repval) || NCDVal_IsInvalid(repval)) {
                    return 0;
                }
                ASSERT(repval.mem == mem)
                
                NCDVal__idx *plptr = buffer_at(mem, instr.placeholder.plidx);
                *plptr = repval.idx;
            } break;
            
            case NCDVAL_INSTR_REINSERT: {
                assert_val_only(mem, instr.reinsert.mapidx);
                struct NCDVal__map *map_e = buffer_at(mem, instr.reinsert.mapidx);
                ASSERT(get_internal_type(map_e->type) == NCDVAL_MAP)
                ASSERT(instr.reinsert.elempos >= 0)
                ASSERT(instr.reinsert.elempos < map_e->count)
                
                NCDVal__MapTreeRef ref = {&map_e->elems[instr.reinsert.elempos], make_map_elem_idx(instr.reinsert.mapidx, instr.reinsert.elempos)};
                NCDVal__MapTree_Remove(&map_e->tree, mem, ref);
                if (!NCDVal__MapTree_Insert(&map_e->tree, mem, ref, NULL)) {
                    BLog(BLOG_ERROR, "duplicate key in map");
                    return 0;
                }
            } break;
            
            case NCDVAL_INSTR_BUMPDEPTH: {
                assert_val_only(mem, instr.bumpdepth.parent_idx);
                int *parent_type_ptr = buffer_at(mem, instr.bumpdepth.parent_idx);
                
                NCDVal__idx *child_type_idx_ptr = buffer_at(mem, instr.bumpdepth.child_idx_idx);
                assert_val_only(mem, *child_type_idx_ptr);
                int *child_type_ptr = buffer_at(mem, *child_type_idx_ptr);
                
                if (!bump_depth(parent_type_ptr, get_depth(*child_type_ptr))) {
                    BLog(BLOG_ERROR, "depth limit exceeded");
                    return 0;
                }
            } break;
            
            default: ASSERT(0);
        }
    }
    
    return 1;
}
