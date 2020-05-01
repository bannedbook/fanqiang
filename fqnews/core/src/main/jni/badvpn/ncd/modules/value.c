/**
 * @file value.c
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
 * Synopsis:
 *   value(value)
 *   value value::get(where)
 *   value value::try_get(where)
 *   value value::getpath(list path)
 *   value value::insert(where, what)
 *   value value::insert(what)
 *   value value::replace(where, what)
 *   value value::replace_this(value)
 *   value value::insert_undo(where, what)
 *   value value::insert_undo(what)
 *   value value::replace_undo(where, what)
 *   value value::replace_this_undo(value)
 * 
 * Description:
 *   Value objects allow examining and manipulating values.
 *   These value objects are actually references to internal value structures, which
 *   may be shared between value objects.
 * 
 *   value(value) constructs a new value object from the given value.
 * 
 *   value::get(where) constructs a value object for the element at position 'where'
 *   (for a list), or the value corresponding to key 'where' (for a map). It is an
 *   error if the base value is not a list or a map, the index is out of bounds of
 *   the list, or the key does not exist in the map.
 *   The resulting value object is NOT a copy, and shares (part of) the same
 *   underlying value structure as the base value object. Deleting it will remove
 *   it from the list or map it is part of.
 * 
 *   value::try_get(where) is like get(), except that if any restriction on 'where'
 *   is violated, no error is triggered; instead, the value object is constructed
 *   as being deleted; this state is exposed via the 'exists' variable.
 *   This can be used to check for the presence of a key in a map, and in case it
 *   exists, allow access to the corresponding value without another get() statement.
 * 
 *   value::getpath(path) is like get(), except that it performs multiple
 *   consecutive resolutions. Also, if the path is an empty list, it performs
 *   no resulution at all.
 * 
 *   value::insert(where, what) constructs a value object by inserting into an
 *   existing value object.
 *   For lists, 'where' is the index of the element to insert before, or the length
 *   of the list to append to it.
 *   For maps, 'where' is the key to insert under. If the key already exists in the
 *   map, its value is replaced; any references to the old value however remain valid.
 * 
 *   value::insert(what) constructs a value object by appending to a list. An error
 *   is triggered if the base value is not a list. Assuming 'list' is a list value,
 *   list->insert(X) is equivalent to list->insert(list.length, X).
 * 
 *   value::replace(where, what) is like insert(), exept that, when inserting into a
 *   list, the value at the specified index is replaced with the new value (unless
 *   the index is equal to the length of the list).
 * 
 *   insert_undo() and replace_undo() are versions of insert() and replace() which
 *   attempt to revert the modifications when they deinitialize.
 *   Specifically, they work like that:
 *   - On initiialization, they take an internal reference to the value being replaced
 *     (if any; note that insert_undo() into a list never replaces a value).
 *   - On deinitialization, they remove the the inserted value from its parent (if there
 *     is one), and insert the old replaced value (to which a reference was kept) in that
 *     place (if any, and assuming it has not been deleted).
 *     Note that if the inserted value changes parents in between init and deinit, the
 *     result of undoing may be unexpected.
 * 
 * Variables:
 *   (empty) - the value stored in the value object
 *   type - type of the value; "string", "list" or "map"
 *   length - number of elements in the list or map, or the number of bytes in a
 *            string
 *   keys - a list of keys in the map (only if the value is a map)
 *   exists - "true" or "false", reflecting whether the value object holds a value
 *            (is not in deleted state)
 * 
 * Synopsis:
 *   value::remove(where)
 *   value::delete()
 * 
 * Description:
 *   value::remove(where) removes from an existing value object.
 *   For lists, 'where' is the index of the element to remove, and must be in range.
 *   For maps, 'where' is the key to remove, and must be an existing key.
 *   In any case, any references to the removed value remain valid.
 * 
 *   value::delete() deletes the underlying value data of this value object.
 *   After delection, the value object enters a deleted state, which will cause any
 *   operation on it to fail. Any other value objects which referred to the same value
 *   or parts of it will too enter deleted state. If the value was an element
 *   in a list or map, is is removed from it.
 * 
 * Synopsis:
 *   value value::substr(string start [, string length])
 * 
 * Description:
 *   Constructs a string value by extracting a part of a string.
 *   'start' specifies the index of the character (from zero) where the substring to
 *   extract starts, and must be <= the length of the string.
 *   'length' specifies the maximum number of characters to extract, if given.
 *   The newly constructed value is a copy of the extracted substring.
 *   The value must be a string value.
 * 
 * Synopsis:
 *   value::reset(what)
 * 
 * Description:
 *   Effectively deconstructs and reconstructs the value object. More precisely,
 *   it builds a new value structure from 'what', possibly invokes a scheduled undo
 *   operation (as scheduled by insert_undo() and replace_undo()), sets up this
 *   value object to reference the newly built value structure, without any scheduled
 *   undo operation.
 * 
 * Synopsis:
 *   value::append(append_val)
 * 
 * Description:
 *   Only defined when the existing value object is a string or list. If it is a string,
 *   appends the string 'append_val' to this string value; 'append_val' must be a string.
 *   If is is a list, inserts 'append_val' to the end of this list value. Unlike insert(),
 *   the resulting append() object is not itself a value object (which in case of insert()
 *   serves as a reference to the new value).
 */

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>
#include <inttypes.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <misc/balloc.h>
#include <structure/LinkedList0.h>
#include <structure/IndexedList.h>
#include <structure/SAvl.h>
#include <ncd/NCDStringIndex.h>
#include <ncd/extra/NCDRefString.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_value.h>

#define STOREDSTRING_TYPE (NCDVAL_STRING | (0 << 3))
#define IDSTRING_TYPE (NCDVAL_STRING | (1 << 3))
#define EXTERNALSTRING_TYPE (NCDVAL_STRING | (2 << 3))

struct value;

#include "value_maptree.h"
#include <structure/SAvl_decl.h>

struct valref {
    struct value *v;
    LinkedList0Node refs_list_node;
};

typedef void (*value_deinit_func) (void *deinit_data, NCDModuleInst *i);

struct instance {
    NCDModuleInst *i;
    struct valref ref;
    value_deinit_func deinit_func;
    void *deinit_data;
};

struct value {
    LinkedList0 refs_list;
    
    struct value *parent;
    union {
        struct {
            IndexedListNode list_contents_il_node;
        } list_parent;
        struct {
            NCDValMem key_mem;
            NCDValRef key;
            MapTreeNode maptree_node;
        } map_parent;
    };
    
    int type;
    union {
        struct {
            NCDRefString *rstr;
            size_t length;
            size_t size;
        } storedstring;
        struct {
            NCD_string_id_t id;
            NCDStringIndex *string_index;
        } idstring;
        struct {
            const char *data;
            size_t length;
            BRefTarget *ref_target;
        } externalstring;
        struct {
            IndexedList list_contents_il;
        } list;
        struct {
            MapTree map_tree;
        } map;
    };
};

static const char * get_type_str (int type);
static void value_cleanup (struct value *v);
static void value_delete (struct value *v);
static struct value * value_init_storedstring (NCDModuleInst *i, MemRef str);
static struct value * value_init_idstring (NCDModuleInst *i, NCD_string_id_t id, NCDStringIndex *string_index);
static struct value * value_init_externalstring (NCDModuleInst *i, MemRef data, BRefTarget *ref_target);
static int value_is_string (struct value *v);
static MemRef value_string_memref (struct value *v);
static void value_string_set_rstr (struct value *v, NCDRefString *rstr, size_t length, size_t size);
static struct value * value_init_list (NCDModuleInst *i);
static size_t value_list_len (struct value *v);
static struct value * value_list_at (struct value *v, size_t index);
static size_t value_list_indexof (struct value *v, struct value *ev);
static int value_list_insert (NCDModuleInst *i, struct value *list, struct value *v, size_t index);
static void value_list_remove (struct value *list, struct value *v);
static struct value * value_init_map (NCDModuleInst *i);
static size_t value_map_len (struct value *map);
static struct value * value_map_at (struct value *map, size_t index);
static struct value * value_map_find (struct value *map, NCDValRef key);
static int value_map_insert (struct value *map, struct value *v, NCDValMem mem, NCDValSafeRef key, NCDModuleInst *i);
static void value_map_remove (struct value *map, struct value *v);
static void value_map_remove2 (struct value *map, struct value *v, NCDValMem *out_mem, NCDValSafeRef *out_key);
static struct value * value_init_fromvalue (NCDModuleInst *i, NCDValRef value);
static int value_to_value (NCDModuleInst *i, struct value *v, NCDValMem *mem, NCDValRef *out_value);
static struct value * value_get (NCDModuleInst *i, struct value *v, NCDValRef where, int no_error);
static struct value * value_get_path (NCDModuleInst *i, struct value *v, NCDValRef path);
static struct value * value_insert (NCDModuleInst *i, struct value *v, NCDValRef where, NCDValRef what, int is_replace, struct value **out_oldv);
static struct value * value_insert_simple (NCDModuleInst *i, struct value *v, NCDValRef what);
static int value_remove (NCDModuleInst *i, struct value *v, NCDValRef where);
static int value_append (NCDModuleInst *i, struct value *v, NCDValRef data);
static void valref_init (struct valref *r, struct value *v);
static void valref_free (struct valref *r);
static struct value * valref_val (struct valref *r);
static void valref_break (struct valref *r);

enum {STRING_EXISTS, STRING_KEYS};

static const char *strings[] = {
    "exists", "keys", NULL
};

#include "value_maptree.h"
#include <structure/SAvl_impl.h>

static const char * get_type_str (int type)
{
    switch (type) {
        case STOREDSTRING_TYPE:
        case IDSTRING_TYPE:
        case EXTERNALSTRING_TYPE:
            return "string";
        case NCDVAL_LIST:
            return "list";
        case NCDVAL_MAP:
            return "map";
    }
    ASSERT(0)
    return NULL;
}

static void value_cleanup (struct value *v)
{
    if (v->parent || !LinkedList0_IsEmpty(&v->refs_list)) {
        return;
    }
    
    switch (v->type) {
        case STOREDSTRING_TYPE: {
            BRefTarget_Deref(NCDRefString_RefTarget(v->storedstring.rstr));
        } break;
        
        case IDSTRING_TYPE: {
        } break;
        
        case EXTERNALSTRING_TYPE: {
            if (v->externalstring.ref_target) {
                BRefTarget_Deref(v->externalstring.ref_target);
            }
        } break;
        
        case NCDVAL_LIST: {
            while (value_list_len(v) > 0) {
                struct value *ev = value_list_at(v, 0);
                value_list_remove(v, ev);
                value_cleanup(ev);
            }
        } break;
        
        case NCDVAL_MAP: {
            while (value_map_len(v) > 0) {
                struct value *ev = value_map_at(v, 0);
                value_map_remove(v, ev);
                value_cleanup(ev);
            }
        } break;
        
        default: ASSERT(0);
    }
    
    free(v);
}

static void value_delete (struct value *v)
{
    if (v->parent) {
        switch (v->parent->type) {
            case NCDVAL_LIST: {
                value_list_remove(v->parent, v);
            } break;
            case NCDVAL_MAP: {
                value_map_remove(v->parent, v);
            } break;
            default: ASSERT(0);
        }
    }
    
    LinkedList0Node *ln;
    while (ln = LinkedList0_GetFirst(&v->refs_list)) {
        struct valref *r = UPPER_OBJECT(ln, struct valref, refs_list_node);
        ASSERT(r->v == v)
        valref_break(r);
    }
    
    switch (v->type) {
        case STOREDSTRING_TYPE: {
            BRefTarget_Deref(NCDRefString_RefTarget(v->storedstring.rstr));
        } break;
        
        case IDSTRING_TYPE: {
        } break;
        
        case EXTERNALSTRING_TYPE: {
            if (v->externalstring.ref_target) {
                BRefTarget_Deref(v->externalstring.ref_target);
            }
        } break;
        
        case NCDVAL_LIST: {
            while (value_list_len(v) > 0) {
                struct value *ev = value_list_at(v, 0);
                value_delete(ev);
            }
        } break;
        
        case NCDVAL_MAP: {
            while (value_map_len(v) > 0) {
                struct value *ev = value_map_at(v, 0);
                value_delete(ev);
            }
        } break;
        
        default: ASSERT(0);
    }
    
    free(v);
}

static struct value * value_init_storedstring (NCDModuleInst *i, MemRef str)
{
    struct value *v = malloc(sizeof(*v));
    if (!v) {
        ModuleLog(i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    LinkedList0_Init(&v->refs_list);
    v->parent = NULL;
    v->type = STOREDSTRING_TYPE;
    
    char *buf;
    if (!(v->storedstring.rstr = NCDRefString_New(str.len, &buf))) {
        ModuleLog(i, BLOG_ERROR, "NCDRefString_New failed");
        goto fail1;
    }
    
    memcpy(buf, str.ptr, str.len);
    
    v->storedstring.length = str.len;
    v->storedstring.size = str.len;
    
    return v;
    
fail1:
    free(v);
fail0:
    return NULL;
}

static struct value * value_init_idstring (NCDModuleInst *i, NCD_string_id_t id, NCDStringIndex *string_index)
{
    ASSERT(string_index == i->params->iparams->string_index)
    
    struct value *v = malloc(sizeof(*v));
    if (!v) {
        ModuleLog(i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    LinkedList0_Init(&v->refs_list);
    v->parent = NULL;
    v->type = IDSTRING_TYPE;
    
    v->idstring.id = id;
    v->idstring.string_index = string_index;
    
    return v;
    
fail0:
    return NULL;
}

static struct value * value_init_externalstring (NCDModuleInst *i, MemRef data, BRefTarget *ref_target)
{
    struct value *v = malloc(sizeof(*v));
    if (!v) {
        ModuleLog(i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    if (ref_target) {
        if (!BRefTarget_Ref(ref_target)) {
            ModuleLog(i, BLOG_ERROR, "BRefTarget_Ref failed");
            goto fail1;
        }
    }
    
    LinkedList0_Init(&v->refs_list);
    v->parent = NULL;
    v->type = EXTERNALSTRING_TYPE;
    
    v->externalstring.data = data.ptr;
    v->externalstring.length = data.len;
    v->externalstring.ref_target = ref_target;
    
    return v;
    
fail1:
    free(v);
fail0:
    return NULL;
}

static int value_is_string (struct value *v)
{
    switch (v->type) {
        case STOREDSTRING_TYPE:
        case IDSTRING_TYPE:
        case EXTERNALSTRING_TYPE:
            return 1;
        default:
            return 0;
    }
}

static MemRef value_string_memref (struct value *v)
{
    ASSERT(value_is_string(v))
    
    switch (v->type) {
        case STOREDSTRING_TYPE:
            return MemRef_Make(NCDRefString_GetBuf(v->storedstring.rstr), v->storedstring.length);
        case IDSTRING_TYPE:
            return NCDStringIndex_Value(v->idstring.string_index, v->idstring.id);
            break;
        case EXTERNALSTRING_TYPE:
            return MemRef_Make(v->externalstring.data, v->externalstring.length);
        default:
            ASSERT(0);
            return MemRef_Make(NULL, 0);
    }
}

static void value_string_set_rstr (struct value *v, NCDRefString *rstr, size_t length, size_t size)
{
    ASSERT(value_is_string(v))
    ASSERT(rstr)
    ASSERT(size >= length)
    
    switch (v->type) {
        case STOREDSTRING_TYPE: {
            BRefTarget_Deref(NCDRefString_RefTarget(v->storedstring.rstr));
        } break;
        
        case IDSTRING_TYPE: {
        } break;
        
        case EXTERNALSTRING_TYPE: {
            if (v->externalstring.ref_target) {
                BRefTarget_Deref(v->externalstring.ref_target);
            }
        } break;
        
        default:
            ASSERT(0);
    }
    
    v->type = STOREDSTRING_TYPE;
    v->storedstring.rstr = rstr;
    v->storedstring.length = length;
    v->storedstring.size = size;
}

static struct value * value_init_list (NCDModuleInst *i)
{
    struct value *v = malloc(sizeof(*v));
    if (!v) {
        ModuleLog(i, BLOG_ERROR, "malloc failed");
        return NULL;
    }
    
    LinkedList0_Init(&v->refs_list);
    v->parent = NULL;
    v->type = NCDVAL_LIST;
    
    IndexedList_Init(&v->list.list_contents_il);
    
    return v;
}

static size_t value_list_len (struct value *v)
{
    ASSERT(v->type == NCDVAL_LIST)
    
    return IndexedList_Count(&v->list.list_contents_il);
}

static struct value * value_list_at (struct value *v, size_t index)
{
    ASSERT(v->type == NCDVAL_LIST)
    ASSERT(index < value_list_len(v))
    
    IndexedListNode *iln = IndexedList_GetAt(&v->list.list_contents_il, index);
    ASSERT(iln)
    
    struct value *e = UPPER_OBJECT(iln, struct value, list_parent.list_contents_il_node);
    ASSERT(e->parent == v)
    
    return e;
}

static size_t value_list_indexof (struct value *v, struct value *ev)
{
    ASSERT(v->type == NCDVAL_LIST)
    ASSERT(ev->parent == v)
    
    uint64_t index = IndexedList_IndexOf(&v->list.list_contents_il, &ev->list_parent.list_contents_il_node);
    ASSERT(index < value_list_len(v))
    
    return index;
}

static int value_list_insert (NCDModuleInst *i, struct value *list, struct value *v, size_t index)
{
    ASSERT(list->type == NCDVAL_LIST)
    ASSERT(!v->parent)
    ASSERT(index <= value_list_len(list))
    
    if (value_list_len(list) == SIZE_MAX) {
        ModuleLog(i, BLOG_ERROR, "list has too many elements");
        return 0;
    }
    
    IndexedList_InsertAt(&list->list.list_contents_il, &v->list_parent.list_contents_il_node, index);
    v->parent = list;
    
    return 1;
}

static void value_list_remove (struct value *list, struct value *v)
{
    ASSERT(list->type == NCDVAL_LIST)
    ASSERT(v->parent == list)
    
    IndexedList_Remove(&list->list.list_contents_il, &v->list_parent.list_contents_il_node);
    v->parent = NULL;
}

static struct value * value_init_map (NCDModuleInst *i)
{
    struct value *v = malloc(sizeof(*v));
    if (!v) {
        ModuleLog(i, BLOG_ERROR, "malloc failed");
        return NULL;
    }
    
    LinkedList0_Init(&v->refs_list);
    v->parent = NULL;
    v->type = NCDVAL_MAP;
    
    MapTree_Init(&v->map.map_tree);
    
    return v;
}

static size_t value_map_len (struct value *map)
{
    ASSERT(map->type == NCDVAL_MAP)
    
    return MapTree_Count(&map->map.map_tree, 0);
}

static struct value * value_map_at (struct value *map, size_t index)
{
    ASSERT(map->type == NCDVAL_MAP)
    ASSERT(index < value_map_len(map))
    
    struct value *e =  MapTree_GetAt(&map->map.map_tree, 0, index);
    ASSERT(e)
    ASSERT(e->parent == map)
    
    return e;
}

static struct value * value_map_find (struct value *map, NCDValRef key)
{
    ASSERT(map->type == NCDVAL_MAP)
    ASSERT(NCDVal_Type(key))
    
    struct value *e = MapTree_LookupExact(&map->map.map_tree, 0, key);
    ASSERT(!e || e->parent == map)
    
    return e;
}

static int value_map_insert (struct value *map, struct value *v, NCDValMem mem, NCDValSafeRef key, NCDModuleInst *i)
{
    ASSERT(map->type == NCDVAL_MAP)
    ASSERT(!v->parent)
    ASSERT((NCDVal_Type(NCDVal_FromSafe(&mem, key)), 1))
    ASSERT(!value_map_find(map, NCDVal_FromSafe(&mem, key)))
    
    if (value_map_len(map) == SIZE_MAX) {
        ModuleLog(i, BLOG_ERROR, "map has too many elements");
        return 0;
    }
    
    v->map_parent.key_mem = mem;
    v->map_parent.key = NCDVal_FromSafe(&v->map_parent.key_mem, key);
    int res = MapTree_Insert(&map->map.map_tree, 0, v, NULL);
    ASSERT_EXECUTE(res)
    v->parent = map;
    
    return 1;
}

static void value_map_remove (struct value *map, struct value *v)
{
    ASSERT(map->type == NCDVAL_MAP)
    ASSERT(v->parent == map)
    
    MapTree_Remove(&map->map.map_tree, 0, v);
    NCDValMem_Free(&v->map_parent.key_mem);
    v->parent = NULL;
}

static void value_map_remove2 (struct value *map, struct value *v, NCDValMem *out_mem, NCDValSafeRef *out_key)
{
    ASSERT(map->type == NCDVAL_MAP)
    ASSERT(v->parent == map)
    ASSERT(out_mem)
    ASSERT(out_key)
    
    MapTree_Remove(&map->map.map_tree, 0, v);
    *out_mem = v->map_parent.key_mem;
    *out_key = NCDVal_ToSafe(v->map_parent.key);
    v->parent = NULL;
}

static struct value * value_init_fromvalue (NCDModuleInst *i, NCDValRef value)
{
    ASSERT((NCDVal_Type(value), 1))
    
    struct value *v;
    
    switch (NCDVal_Type(value)) {
        case NCDVAL_STRING: {
            if (NCDVal_IsIdString(value)) {
                v = value_init_idstring(i, NCDVal_IdStringId(value), NCDValMem_StringIndex(value.mem));
            } else if (NCDVal_IsExternalString(value)) {
                v = value_init_externalstring(i, NCDVal_StringMemRef(value), NCDVal_ExternalStringTarget(value));
            } else {
                v = value_init_storedstring(i, NCDVal_StringMemRef(value));
            }
            if (!v) {
                goto fail0;
            }
        } break;
        
        case NCDVAL_LIST: {
            if (!(v = value_init_list(i))) {
                goto fail0;
            }
            
            size_t count = NCDVal_ListCount(value);
            
            for (size_t j = 0; j < count; j++) {
                struct value *ev = value_init_fromvalue(i, NCDVal_ListGet(value, j));
                if (!ev) {
                    goto fail1;
                }
                
                if (!value_list_insert(i, v, ev, value_list_len(v))) {
                    value_cleanup(ev);
                    goto fail1;
                }
            }
        } break;
        
        case NCDVAL_MAP: {
            if (!(v = value_init_map(i))) {
                goto fail0;
            }
            
            for (NCDValMapElem e = NCDVal_MapFirst(value); !NCDVal_MapElemInvalid(e); e = NCDVal_MapNext(value, e)) {
                NCDValRef ekey = NCDVal_MapElemKey(value, e);
                NCDValRef eval = NCDVal_MapElemVal(value, e);
                
                NCDValMem key_mem;
                NCDValMem_Init(&key_mem, i->params->iparams->string_index);
                
                NCDValRef key = NCDVal_NewCopy(&key_mem, ekey);
                if (NCDVal_IsInvalid(key)) {
                    NCDValMem_Free(&key_mem);
                    goto fail1;
                }
                
                struct value *ev = value_init_fromvalue(i, eval);
                if (!ev) {
                    NCDValMem_Free(&key_mem);
                    goto fail1;
                }
                
                if (!value_map_insert(v, ev, key_mem, NCDVal_ToSafe(key), i)) {
                    NCDValMem_Free(&key_mem);
                    value_cleanup(ev);
                    goto fail1;
                }
            }
        } break;
        
        default:
            ASSERT(0);
            return NULL;
    }
    
    return v;
    
fail1:
    value_cleanup(v);
fail0:
    return NULL;
}

static int value_to_value (NCDModuleInst *i, struct value *v, NCDValMem *mem, NCDValRef *out_value)
{
    ASSERT(mem)
    ASSERT(out_value)
    
    switch (v->type) {
        case STOREDSTRING_TYPE: {
            *out_value = NCDVal_NewExternalString(mem, NCDRefString_GetBuf(v->storedstring.rstr), v->storedstring.length, NCDRefString_RefTarget(v->storedstring.rstr));
            if (NCDVal_IsInvalid(*out_value)) {
                goto fail;
            }
        } break;
        
        case IDSTRING_TYPE: {
            *out_value = NCDVal_NewIdString(mem, v->idstring.id);
            if (NCDVal_IsInvalid(*out_value)) {
                goto fail;
            }
        } break;
        
        case EXTERNALSTRING_TYPE: {
            *out_value = NCDVal_NewExternalString(mem, v->externalstring.data, v->externalstring.length, v->externalstring.ref_target);
            if (NCDVal_IsInvalid(*out_value)) {
                goto fail;
            }
        } break;
        
        case NCDVAL_LIST: {
            *out_value = NCDVal_NewList(mem, value_list_len(v));
            if (NCDVal_IsInvalid(*out_value)) {
                goto fail;
            }
            
            for (size_t index = 0; index < value_list_len(v); index++) {
                NCDValRef eval;
                if (!value_to_value(i, value_list_at(v, index), mem, &eval)) {
                    goto fail;
                }
                
                if (!NCDVal_ListAppend(*out_value, eval)) {
                    goto fail;
                }
            }
        } break;
        
        case NCDVAL_MAP: {
            *out_value = NCDVal_NewMap(mem, value_map_len(v));
            if (NCDVal_IsInvalid(*out_value)) {
                goto fail;
            }
            
            for (size_t index = 0; index < value_map_len(v); index++) {
                struct value *ev = value_map_at(v, index);
                
                NCDValRef key = NCDVal_NewCopy(mem, ev->map_parent.key);
                if (NCDVal_IsInvalid(key)) {
                    goto fail;
                }
                
                NCDValRef val;
                if (!value_to_value(i, ev, mem, &val)) {
                    goto fail;
                }
                
                int inserted;
                if (!NCDVal_MapInsert(*out_value, key, val, &inserted)) {
                    goto fail;
                }
                ASSERT_EXECUTE(inserted)
            }
        } break;
        
        default: ASSERT(0);
    }
    
    return 1;
    
fail:
    return 0;
}

static struct value * value_get (NCDModuleInst *i, struct value *v, NCDValRef where, int no_error)
{
    ASSERT((NCDVal_Type(where), 1))
    
    switch (v->type) {
        case STOREDSTRING_TYPE:
        case IDSTRING_TYPE:
        case EXTERNALSTRING_TYPE: {
            if (!no_error) ModuleLog(i, BLOG_ERROR, "cannot resolve into a string");
            goto fail;
        } break;
        
        case NCDVAL_LIST: {
            uintmax_t index;
            if (!ncd_read_uintmax(where, &index)) {
                if (!no_error) ModuleLog(i, BLOG_ERROR, "index is not a valid number (resolving into list)");
                goto fail;
            }
            
            if (index >= value_list_len(v)) {
                if (!no_error) ModuleLog(i, BLOG_ERROR, "index is out of bounds (resolving into list)");
                goto fail;
            }
            
            v = value_list_at(v, index);
        } break;
        
        case NCDVAL_MAP: {
            v = value_map_find(v, where);
            if (!v) {
                if (!no_error) ModuleLog(i, BLOG_ERROR, "key does not exist (resolving into map)");
                goto fail;
            }
        } break;
        
        default: ASSERT(0);
    }
    
    return v;
    
fail:
    return NULL;
}

static struct value * value_get_path (NCDModuleInst *i, struct value *v, NCDValRef path)
{
    ASSERT(NCDVal_IsList(path))
    
    size_t count = NCDVal_ListCount(path);
    
    for (size_t j = 0; j < count; j++) {
        if (!(v = value_get(i, v, NCDVal_ListGet(path, j), 0))) {
            goto fail;
        }
    }
    
    return v;
    
fail:
    return NULL;
}

static struct value * value_insert (NCDModuleInst *i, struct value *v, NCDValRef where, NCDValRef what, int is_replace, struct value **out_oldv)
{
    ASSERT(v)
    ASSERT((NCDVal_Type(where), 1))
    ASSERT((NCDVal_Type(what), 1))
    ASSERT(is_replace == !!is_replace)
    
    struct value *nv = value_init_fromvalue(i, what);
    if (!nv) {
        goto fail0;
    }
    
    struct value *oldv = NULL;
    
    switch (v->type) {
        case STOREDSTRING_TYPE:
        case IDSTRING_TYPE:
        case EXTERNALSTRING_TYPE: {
            ModuleLog(i, BLOG_ERROR, "cannot insert into a string");
            goto fail1;
        } break;
        
        case NCDVAL_LIST: {
            uintmax_t index;
            if (!ncd_read_uintmax(where, &index)) {
                ModuleLog(i, BLOG_ERROR, "index is not a valid number (inserting into list)");
                goto fail1;
            }
            
            if (index > value_list_len(v)) {
                ModuleLog(i, BLOG_ERROR, "index is out of bounds (inserting into list)");
                goto fail1;
            }
            
            if (is_replace && index < value_list_len(v)) {
                oldv = value_list_at(v, index);
                
                value_list_remove(v, oldv);
                
                int res = value_list_insert(i, v, nv, index);
                ASSERT_EXECUTE(res)
            } else {
                if (!value_list_insert(i, v, nv, index)) {
                    goto fail1;
                }
            }
        } break;
        
        case NCDVAL_MAP: {
            oldv = value_map_find(v, where);
            
            if (!oldv && value_map_len(v) == SIZE_MAX) {
                ModuleLog(i, BLOG_ERROR, "map has too many elements");
                goto fail1;
            }
            
            NCDValMem key_mem;
            NCDValMem_Init(&key_mem, i->params->iparams->string_index);
            
            NCDValRef key = NCDVal_NewCopy(&key_mem, where);
            if (NCDVal_IsInvalid(key)) {
                NCDValMem_Free(&key_mem);
                goto fail1;
            }
            
            if (oldv) {
                value_map_remove(v, oldv);
            }
            
            int res = value_map_insert(v, nv, key_mem, NCDVal_ToSafe(key), i);
            ASSERT_EXECUTE(res)
        } break;
        
        default: ASSERT(0);
    }
    
    if (out_oldv) {
        *out_oldv = oldv;
    }
    else if (oldv) {
        value_cleanup(oldv);
    }
    
    return nv;
    
fail1:
    value_cleanup(nv);
fail0:
    return NULL;
}

static struct value * value_insert_simple (NCDModuleInst *i, struct value *v, NCDValRef what)
{
    ASSERT(v)
    ASSERT((NCDVal_Type(what), 1))
    
    struct value *nv = value_init_fromvalue(i, what);
    if (!nv) {
        goto fail0;
    }
    
    switch (v->type) {
        case NCDVAL_LIST: {
            if (!value_list_insert(i, v, nv, value_list_len(v))) {
                goto fail1;
            }
        } break;
        
        default:
            ModuleLog(i, BLOG_ERROR, "one-argument insert is only defined for lists");
            return NULL;
    }
    
    return nv;
    
fail1:
    value_cleanup(nv);
fail0:
    return NULL;
}

static int value_remove (NCDModuleInst *i, struct value *v, NCDValRef where)
{
    ASSERT(v)
    ASSERT((NCDVal_Type(where), 1))
    
    switch (v->type) {
        case STOREDSTRING_TYPE:
        case IDSTRING_TYPE:
        case EXTERNALSTRING_TYPE: {
            ModuleLog(i, BLOG_ERROR, "cannot remove from a string");
            goto fail;
        } break;
        
        case NCDVAL_LIST: {
            uintmax_t index;
            if (!ncd_read_uintmax(where, &index)) {
                ModuleLog(i, BLOG_ERROR, "index is not a valid number (removing from list)");
                goto fail;
            }
            
            if (index >= value_list_len(v)) {
                ModuleLog(i, BLOG_ERROR, "index is out of bounds (removing from list)");
                goto fail;
            }
            
            struct value *ov = value_list_at(v, index);
            
            value_list_remove(v, ov);
            value_cleanup(ov);
        } break;
        
        case NCDVAL_MAP: {
            struct value *ov = value_map_find(v, where);
            if (!ov) {
                ModuleLog(i, BLOG_ERROR, "key does not exist (removing from map)");
                goto fail;
            }
            
            value_map_remove(v, ov);
            value_cleanup(ov);
        } break;
        
        default: ASSERT(0);
    }
    
    return 1;
    
fail:
    return 0;
}

static int value_append (NCDModuleInst *i, struct value *v, NCDValRef data)
{
    ASSERT(v)
    ASSERT((NCDVal_Type(data), 1))
    
    switch (v->type) {
        case STOREDSTRING_TYPE:
        case IDSTRING_TYPE:
        case EXTERNALSTRING_TYPE: {
            if (!NCDVal_IsString(data)) {
                ModuleLog(i, BLOG_ERROR, "cannot append non-string to string");
                return 0;
            }
            
            MemRef v_str = value_string_memref(v);
            size_t append_length = NCDVal_StringLength(data);
            
            if (append_length > SIZE_MAX - v_str.len) {
                ModuleLog(i, BLOG_ERROR, "too much data to append");
                return 0;
            }
            size_t new_length = v_str.len + append_length;
            
            if (v->type == STOREDSTRING_TYPE && new_length <= v->storedstring.size) {
                char *existing_buf = (char *)NCDRefString_GetBuf(v->storedstring.rstr);
                NCDVal_StringCopyOut(data, 0, append_length, existing_buf + v_str.len);
                v->storedstring.length = new_length;
            } else {
                // only allocate power-of-two sizez
                size_t new_size = 16;
                while (new_size < new_length) {
                    if (new_size > SIZE_MAX / 2) {
                        ModuleLog(i, BLOG_ERROR, "too much data to append");
                        return 0;
                    }
                    new_size *= 2;
                }
                
                char *new_buf;
                NCDRefString *new_rstr = NCDRefString_New(new_size, &new_buf);
                if (!new_rstr) {
                    ModuleLog(i, BLOG_ERROR, "NCDRefString_New failed");
                    return 0;
                }
                
                MemRef_CopyOut(v_str, new_buf);
                NCDVal_StringCopyOut(data, 0, append_length, new_buf + v_str.len);
                
                value_string_set_rstr(v, new_rstr, new_length, new_size);
            }
        } break;
        
        case NCDVAL_LIST: {
            struct value *nv = value_init_fromvalue(i, data);
            if (!nv) {
                return 0;
            }
            
            if (!value_list_insert(i, v, nv, value_list_len(v))) {
                value_cleanup(nv);
                return 0;
            }
        } break;
        
        default:
            ModuleLog(i, BLOG_ERROR, "append is only defined for strings and lists");
            return 0;
    }
    
    return 1;
}

static void valref_init (struct valref *r, struct value *v)
{
    r->v = v;
    
    if (v) {
        LinkedList0_Prepend(&v->refs_list, &r->refs_list_node);
    }
}

static void valref_free (struct valref *r)
{
    if (r->v) {
        LinkedList0_Remove(&r->v->refs_list, &r->refs_list_node);
        value_cleanup(r->v);
    }
}

static struct value * valref_val (struct valref *r)
{
    return r->v;
}

static void valref_break (struct valref *r)
{
    ASSERT(r->v)
    
    LinkedList0_Remove(&r->v->refs_list, &r->refs_list_node);
    r->v = NULL;
}

static void func_new_common (void *vo, NCDModuleInst *i, struct value *v, value_deinit_func deinit_func, void *deinit_data)
{
    struct instance *o = vo;
    o->i = i;
    
    // init value references
    valref_init(&o->ref, v);
    
    // remember deinit
    o->deinit_func = deinit_func;
    o->deinit_data = deinit_data;
    
    NCDModuleInst_Backend_Up(i);
    return;
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    // deinit
    if (o->deinit_func) {
        o->deinit_func(o->deinit_data, o->i);
    }
    
    // free value reference
    valref_free(&o->ref);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    struct value *v = valref_val(&o->ref);
    
    if (name == ModuleString(o->i, STRING_EXISTS)) {
        *out = ncd_make_boolean(mem, !!v);
        return 1;
    }
    
    if (name != NCD_STRING_TYPE && name != NCD_STRING_LENGTH &&
        name != ModuleString(o->i, STRING_KEYS) && name != NCD_STRING_EMPTY) {
        return 0;
    }
    
    if (!v) {
        ModuleLog(o->i, BLOG_ERROR, "value was deleted");
        return 0;
    }
    
    if (name == NCD_STRING_TYPE) {
        *out = NCDVal_NewString(mem, get_type_str(v->type));
    }
    else if (name == NCD_STRING_LENGTH) {
        size_t len = 0; // to remove warning
        switch (v->type) {
            case NCDVAL_LIST:
                len = value_list_len(v);
                break;
            case NCDVAL_MAP:
                len = value_map_len(v);
                break;
            default:
                ASSERT(value_is_string(v))
                len = value_string_memref(v).len;
                break;
        }
        
        *out = ncd_make_uintmax(mem, len);
    }
    else if (name == ModuleString(o->i, STRING_KEYS)) {
        if (v->type != NCDVAL_MAP) {
            ModuleLog(o->i, BLOG_ERROR, "value is not a map (reading keys variable)");
            return 0;
        }
        
        *out = NCDVal_NewList(mem, value_map_len(v));
        if (NCDVal_IsInvalid(*out)) {
            goto fail;
        }
        
        for (size_t j = 0; j < value_map_len(v); j++) {
            struct value *ev = value_map_at(v, j);
            
            NCDValRef key = NCDVal_NewCopy(mem, ev->map_parent.key);
            if (NCDVal_IsInvalid(key)) {
                goto fail;
            }
            
            if (!NCDVal_ListAppend(*out, key)) {
                goto fail;
            }
        }
    }
    else if (name == NCD_STRING_EMPTY) {
        if (!value_to_value(o->i, v, mem, out)) {
            return 0;
        }
    }
    else {
        ASSERT(0);
    }
    
    return 1;
    
fail:
    *out = NCDVal_NewInvalid();
    return 1;
}

static void func_new_value (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef value_arg;
    if (!NCDVal_ListRead(params->args, 1, &value_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct value *v = value_init_fromvalue(i, value_arg);
    if (!v) {
        goto fail0;
    }
    
    func_new_common(vo, i, v, NULL, NULL);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_get (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef where_arg;
    if (!NCDVal_ListRead(params->args, 1, &where_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    struct value *v = value_get(i, mov, where_arg, 0);
    if (!v) {
        goto fail0;
    }
    
    func_new_common(vo, i, v, NULL, NULL);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_try_get (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef where_arg;
    if (!NCDVal_ListRead(params->args, 1, &where_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    struct value *v = value_get(i, mov, where_arg, 1);
    
    func_new_common(vo, i, v, NULL, NULL);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_getpath (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef path_arg;
    if (!NCDVal_ListRead(params->args, 1, &path_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsList(path_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    struct value *v = value_get_path(i, mov, path_arg);
    if (!v) {
        goto fail0;
    }
    
    func_new_common(vo, i, v, NULL, NULL);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_insert_replace_common (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int is_replace)
{
    NCDValRef where_arg = NCDVal_NewInvalid();
    NCDValRef what_arg;
    if (!(!is_replace && NCDVal_ListRead(params->args, 1, &what_arg)) &&
        !NCDVal_ListRead(params->args, 2, &where_arg, &what_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    struct value *v;
    
    if (NCDVal_IsInvalid(where_arg)) {
        v = value_insert_simple(i, mov, what_arg);
    } else {
        v = value_insert(i, mov, where_arg, what_arg, is_replace, NULL);
    }
    if (!v) {
        goto fail0;
    }
    
    func_new_common(vo, i, v, NULL, NULL);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_insert (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new_insert_replace_common(vo, i, params, 0);
}

static void func_new_replace (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new_insert_replace_common(vo, i, params, 1);
}

struct insert_undo_deinit_data {
    struct valref val_ref;
    struct valref oldval_ref;
};

static void undo_deinit_func (struct insert_undo_deinit_data *data, NCDModuleInst *i)
{
    struct value *val = valref_val(&data->val_ref);
    struct value *oldval = valref_val(&data->oldval_ref);
    
    if (val && val->parent && (!oldval || !oldval->parent)) {
        // get parent
        struct value *parent = val->parent;
        
        // remove this value from parent and restore saved one (or none)
        switch (parent->type) {
            case NCDVAL_LIST: {
                size_t index = value_list_indexof(parent, val);
                value_list_remove(parent, val);
                if (oldval) {
                    int res = value_list_insert(i, parent, oldval, index);
                    ASSERT_EXECUTE(res)
                }
            } break;
            
            case NCDVAL_MAP: {
                NCDValMem key_mem;
                NCDValSafeRef key;
                value_map_remove2(parent, val, &key_mem, &key);
                if (oldval) {
                    int res = value_map_insert(parent, oldval, key_mem, key, i);
                    ASSERT_EXECUTE(res)
                } else {
                    NCDValMem_Free(&key_mem);
                }
            } break;
            
            default: ASSERT(0);
        }
    }
    
    valref_free(&data->oldval_ref);
    valref_free(&data->val_ref);
    free(data);
}

static void func_new_insert_replace_undo_common (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int is_replace)
{
    NCDValRef where_arg = NCDVal_NewInvalid();
    NCDValRef what_arg;
    if (!(!is_replace && NCDVal_ListRead(params->args, 1, &what_arg)) &&
        !NCDVal_ListRead(params->args, 2, &where_arg, &what_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    struct insert_undo_deinit_data *data = malloc(sizeof(*data));
    if (!data) {
        ModuleLog(i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    struct value *oldv;
    struct value *v;
    
    if (NCDVal_IsInvalid(where_arg)) {
        oldv = NULL;
        v = value_insert_simple(i, mov, what_arg);
    } else {
        v = value_insert(i, mov, where_arg, what_arg, is_replace, &oldv);
    }
    if (!v) {
        goto fail1;
    }
    
    valref_init(&data->val_ref, v);
    valref_init(&data->oldval_ref, oldv);
    
    func_new_common(vo, i, v, (value_deinit_func)undo_deinit_func, data);
    return;
    
fail1:
    free(data);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_insert_undo (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new_insert_replace_undo_common(vo, i, params, 0);
}

static void func_new_replace_undo (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new_insert_replace_undo_common(vo, i, params, 1);
}

static void func_new_replace_this (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef value_arg;
    if (!NCDVal_ListRead(params->args, 1, &value_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    struct value *v = value_init_fromvalue(i, value_arg);
    if (!v) {
        goto fail0;
    }
    
    if (mov->parent) {
        struct value *parent = mov->parent;
        
        switch (parent->type) {
            case NCDVAL_LIST: {
                size_t index = value_list_indexof(parent, mov);
                value_list_remove(parent, mov);
                int res = value_list_insert(i, parent, v, index);
                ASSERT_EXECUTE(res)
            } break;
            
            case NCDVAL_MAP: {
                NCDValMem key_mem;
                NCDValSafeRef key;
                value_map_remove2(parent, mov, &key_mem, &key);
                int res = value_map_insert(parent, v, key_mem, key, i);
                ASSERT_EXECUTE(res)
            } break;
            
            default: ASSERT(0);
        }
        
        value_cleanup(mov);
    }
    
    func_new_common(vo, i, v, NULL, NULL);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_replace_this_undo (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef value_arg;
    if (!NCDVal_ListRead(params->args, 1, &value_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    struct value *v = value_init_fromvalue(i, value_arg);
    if (!v) {
        goto fail0;
    }
    
    struct insert_undo_deinit_data *data = malloc(sizeof(*data));
    if (!data) {
        ModuleLog(i, BLOG_ERROR, "malloc failed");
        goto fail1;
    }
    
    valref_init(&data->val_ref, v);
    valref_init(&data->oldval_ref, mov);
    
    if (mov->parent) {
        struct value *parent = mov->parent;
        
        switch (parent->type) {
            case NCDVAL_LIST: {
                size_t index = value_list_indexof(parent, mov);
                value_list_remove(parent, mov);
                int res = value_list_insert(i, parent, v, index);
                ASSERT_EXECUTE(res)
            } break;
            
            case NCDVAL_MAP: {
                NCDValMem key_mem;
                NCDValSafeRef key;
                value_map_remove2(parent, mov, &key_mem, &key);
                int res = value_map_insert(parent, v, key_mem, key, i);
                ASSERT_EXECUTE(res)
            } break;
            
            default: ASSERT(0);
        }
    }
    
    func_new_common(vo, i, v, (value_deinit_func)undo_deinit_func, data);
    return;
    
fail1:
    value_cleanup(v);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_substr (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef start_arg;
    NCDValRef length_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 1, &start_arg) &&
        !NCDVal_ListRead(params->args, 2, &start_arg, &length_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    uintmax_t start;
    if (!ncd_read_uintmax(start_arg, &start)) {
        ModuleLog(i, BLOG_ERROR, "start is not a number");
        goto fail0;
    }
    
    uintmax_t length = UINTMAX_MAX;
    if (!NCDVal_IsInvalid(length_arg) && !ncd_read_uintmax(length_arg, &length)) {
        ModuleLog(i, BLOG_ERROR, "length is not a number");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    if (!value_is_string(mov)) {
        ModuleLog(i, BLOG_ERROR, "value is not a string");
        goto fail0;
    }
    
    MemRef string = value_string_memref(mov);
    
    if (start > string.len) {
        ModuleLog(i, BLOG_ERROR, "start is out of range");
        goto fail0;
    }
    
    size_t remain = string.len - start;
    size_t amount = length < remain ? length : remain;
    
    MemRef sub_string = MemRef_Sub(string, start, amount);
    
    struct value *v = NULL;
    switch (mov->type) {
        case STOREDSTRING_TYPE: {
            v = value_init_externalstring(i, sub_string, NCDRefString_RefTarget(mov->storedstring.rstr));
        } break;
        case IDSTRING_TYPE: {
            v = value_init_storedstring(i, sub_string);
        } break;
        case EXTERNALSTRING_TYPE: {
            v = value_init_externalstring(i, sub_string, mov->externalstring.ref_target);
        } break;
        default:
            ASSERT(0);
    }
    
    if (!v) {
        goto fail0;
    }
    
    func_new_common(vo, i, v, NULL, NULL);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void remove_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef where_arg;
    if (!NCDVal_ListRead(params->args, 1, &where_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    if (!value_remove(i, mov, where_arg)) {
        goto fail0;
    }
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void delete_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    value_delete(mov);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void reset_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef what_arg;
    if (!NCDVal_ListRead(params->args, 1, &what_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // build value from argument
    struct value *newv = value_init_fromvalue(i, what_arg);
    if (!newv) {
        goto fail0;
    }
    
    // deinit
    if (mo->deinit_func) {
        mo->deinit_func(mo->deinit_data, i);
    }
    
    // free value reference
    valref_free(&mo->ref);
    
    // set up value reference
    valref_init(&mo->ref, newv);
    
    // set no deinit function
    mo->deinit_func = NULL;
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void append_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef data_arg;
    if (!NCDVal_ListRead(params->args, 1, &data_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    struct value *mov = valref_val(&mo->ref);
    
    if (!mov) {
        ModuleLog(i, BLOG_ERROR, "value was deleted");
        goto fail0;
    }
    
    if (!value_append(i, mov, data_arg)) {
        goto fail0;
    }
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "value",
        .func_new2 = func_new_value,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::get",
        .base_type = "value",
        .func_new2 = func_new_get,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::try_get",
        .base_type = "value",
        .func_new2 = func_new_try_get,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::getpath",
        .base_type = "value",
        .func_new2 = func_new_getpath,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::insert",
        .base_type = "value",
        .func_new2 = func_new_insert,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::replace",
        .base_type = "value",
        .func_new2 = func_new_replace,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::replace_this",
        .base_type = "value",
        .func_new2 = func_new_replace_this,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::insert_undo",
        .base_type = "value",
        .func_new2 = func_new_insert_undo,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::replace_undo",
        .base_type = "value",
        .func_new2 = func_new_replace_undo,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::replace_this_undo",
        .base_type = "value",
        .func_new2 = func_new_replace_this_undo,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::remove",
        .func_new2 = remove_func_new
    }, {
        .type = "value::delete",
        .func_new2 = delete_func_new
    }, {
        .type = "value::reset",
        .func_new2 = reset_func_new
    }, {
        .type = "value::substr",
        .base_type = "value",
        .func_new2 = func_new_substr,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "value::append",
        .func_new2 = append_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_value = {
    .modules = modules,
    .strings = strings
};
