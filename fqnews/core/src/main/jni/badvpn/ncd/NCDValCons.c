/**
 * @file NCDValCons.c
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

#include <misc/balloc.h>

#include "NCDValCons.h"

#define GROWARRAY_NAME NCDValCons__Array
#define GROWARRAY_OBJECT_TYPE NCDValCons
#define GROWARRAY_ARRAY_MEMBER elems
#define GROWARRAY_CAPACITY_MEMBER elems_capacity
#define GROWARRAY_MAX_CAPACITY INT_MAX
#include <misc/grow_array.h>

static int alloc_elem (NCDValCons *o)
{
    ASSERT(o->elems_size >= 0)
    ASSERT(o->elems_size <= o->elems_capacity)
    
    if (o->elems_size == o->elems_capacity && !NCDValCons__Array_DoubleUp(o)) {
        return -1;
    }
    
    return o->elems_size++;
}

static void assert_cons_val (NCDValCons *o, NCDValConsVal val)
{
#ifndef NDEBUG
    switch (val.cons_type) {
        case NCDVALCONS_TYPE_COMPLETE: {
            ASSERT(!NCDVal_IsInvalid(NCDVal_FromSafe(o->mem, val.u.complete_ref)))
        } break;
        case NCDVALCONS_TYPE_INCOMPLETE_LIST:
        case NCDVALCONS_TYPE_INCOMPLETE_MAP: {
            ASSERT(val.u.incomplete.elems_idx >= -1)
            ASSERT(val.u.incomplete.elems_idx < o->elems_size)
            ASSERT(val.u.incomplete.count >= 0)
        } break;
        default:
            ASSERT(0);
    }
#endif
}

static int complete_value (NCDValCons *o, NCDValConsVal val, NCDValSafeRef *out, int *out_error)
{
    assert_cons_val(o, val);
    ASSERT(out)
    ASSERT(out_error)
    
    switch (val.cons_type) {
        case NCDVALCONS_TYPE_COMPLETE: {
            *out = val.u.complete_ref;
        } break;
        
        case NCDVALCONS_TYPE_INCOMPLETE_LIST: {
            NCDValRef list = NCDVal_NewList(o->mem, val.u.incomplete.count);
            if (NCDVal_IsInvalid(list)) {
                goto fail_memory;
            }
            
            int elemidx = val.u.incomplete.elems_idx;
            
            while (elemidx != -1) {
                ASSERT(elemidx >= 0)
                ASSERT(elemidx < o->elems_size)
                
                NCDValRef elem = NCDVal_FromSafe(o->mem, o->elems[elemidx].ref);
                
                if (!NCDVal_ListAppend(list, elem)) {
                    *out_error = NCDVALCONS_ERROR_DEPTH;
                    return 0;
                }
                
                elemidx = o->elems[elemidx].next;
            }
            
            *out = NCDVal_ToSafe(list);
        } break;
        
        case NCDVALCONS_TYPE_INCOMPLETE_MAP: {
            NCDValRef map = NCDVal_NewMap(o->mem, val.u.incomplete.count);
            if (NCDVal_IsInvalid(map)) {
                goto fail_memory;
            }
            
            int keyidx = val.u.incomplete.elems_idx;
            
            while (keyidx != -1) {
                ASSERT(keyidx >= 0)
                ASSERT(keyidx < o->elems_size)
                
                int validx = o->elems[keyidx].next;
                ASSERT(validx >= 0)
                ASSERT(validx < o->elems_size)
                
                NCDValRef key = NCDVal_FromSafe(o->mem, o->elems[keyidx].ref);
                NCDValRef value = NCDVal_FromSafe(o->mem, o->elems[validx].ref);
                
                int inserted;
                if (!NCDVal_MapInsert(map, key, value, &inserted)) {
                    *out_error = NCDVALCONS_ERROR_DEPTH;
                    return 0;
                }
                if (!inserted) {
                    *out_error = NCDVALCONS_ERROR_DUPLICATE_KEY;
                    return 0;
                }
                
                keyidx = o->elems[validx].next;
            }
            
            *out = NCDVal_ToSafe(map);
        } break;
        
        default:
            ASSERT(0);
    }
    
    return 1;
    
fail_memory:
    *out_error = NCDVALCONS_ERROR_MEMORY;
    return 0;
}

int NCDValCons_Init (NCDValCons *o, NCDValMem *mem)
{
    ASSERT(mem)
    
    o->mem = mem;
    o->elems_size = 0;
    
    if (!NCDValCons__Array_Init(o, 1)) {
        return 0;
    }
    
    return 1;
}

void NCDValCons_Free (NCDValCons *o)
{
    NCDValCons__Array_Free(o);
}

int NCDValCons_NewString (NCDValCons *o, const uint8_t *data, size_t len, NCDValConsVal *out, int *out_error)
{
    ASSERT(out)
    ASSERT(out_error)
    
    NCDValRef ref = NCDVal_NewStringBin(o->mem, data, len);
    if (NCDVal_IsInvalid(ref)) {
        *out_error = NCDVALCONS_ERROR_MEMORY;
        return 0;
    }
    
    out->cons_type = NCDVALCONS_TYPE_COMPLETE;
    out->u.complete_ref = NCDVal_ToSafe(ref);
    
    return 1;
}

void NCDValCons_NewList (NCDValCons *o, NCDValConsVal *out)
{
    ASSERT(out)
    
    out->cons_type = NCDVALCONS_TYPE_INCOMPLETE_LIST;
    out->u.incomplete.elems_idx = -1;
    out->u.incomplete.count = 0;
}

void NCDValCons_NewMap (NCDValCons *o, NCDValConsVal *out)
{
    ASSERT(out)
    
    out->cons_type = NCDVALCONS_TYPE_INCOMPLETE_MAP;
    out->u.incomplete.elems_idx = -1;
    out->u.incomplete.count = 0;
}

int NCDValCons_ListPrepend (NCDValCons *o, NCDValConsVal *list, NCDValConsVal elem, int *out_error)
{
    assert_cons_val(o, *list);
    ASSERT(list->cons_type == NCDVALCONS_TYPE_INCOMPLETE_LIST)
    assert_cons_val(o, elem);
    ASSERT(out_error)
    
    int elemidx = alloc_elem(o);
    if (elemidx < 0) {
        *out_error = NCDVALCONS_ERROR_MEMORY;
        return 0;
    }
    
    o->elems[elemidx].next = list->u.incomplete.elems_idx;
    
    if (!complete_value(o, elem, &o->elems[elemidx].ref, out_error)) {
        return 0;
    }
    
    list->u.incomplete.elems_idx = elemidx;
    list->u.incomplete.count++;
    
    return 1;
}

int NCDValCons_MapInsert (NCDValCons *o, NCDValConsVal *map, NCDValConsVal key, NCDValConsVal value, int *out_error)
{
    assert_cons_val(o, *map);
    ASSERT(map->cons_type == NCDVALCONS_TYPE_INCOMPLETE_MAP)
    assert_cons_val(o, key);
    assert_cons_val(o, value);
    ASSERT(out_error)
    
    int validx = alloc_elem(o);
    if (validx < 0) {
        *out_error = NCDVALCONS_ERROR_MEMORY;
        return 0;
    }
    
    int keyidx = alloc_elem(o);
    if (keyidx < 0) {
        *out_error = NCDVALCONS_ERROR_MEMORY;
        return 0;
    }
    
    o->elems[validx].next = map->u.incomplete.elems_idx;
    o->elems[keyidx].next = validx;
    
    if (!complete_value(o, value, &o->elems[validx].ref, out_error)) {
        return 0;
    }
    
    if (!complete_value(o, key, &o->elems[keyidx].ref, out_error)) {
        return 0;
    }
    
    map->u.incomplete.elems_idx = keyidx;
    map->u.incomplete.count++;
    
    return 1;
}

int NCDValCons_Complete (NCDValCons *o, NCDValConsVal val, NCDValRef *out, int *out_error)
{
    assert_cons_val(o, val);
    ASSERT(out)
    ASSERT(out_error)
    
    NCDValSafeRef sref;
    if (!complete_value(o, val, &sref, out_error)) {
        return 0;
    }
    
    *out = NCDVal_FromSafe(o->mem, sref);
    return 1;
}
