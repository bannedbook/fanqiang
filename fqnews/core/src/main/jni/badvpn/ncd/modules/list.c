/**
 * @file list.c
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
 * List construction module.
 * 
 * Synopsis:
 *   list(elem1, ..., elemN)
 *   list listfrom(list l1, ..., list lN)
 * 
 * Description:
 *   The first form creates a list with the given elements.
 *   The second form creates a list by concatenating the given
 *   lists.
 * 
 * Variables:
 *   (empty) - list containing elem1, ..., elemN
 *   length - number of elements in list
 * 
 * Synopsis: list::append(arg)
 * 
 * Synopsis: list::appendv(list arg)
 * Description: Appends the elements of arg to the list.
 * 
 * Synopsis: list::length()
 * Variables:
 *   (empty) - number of elements in list at the time of initialization
 *             of this method
 * 
 * Synopsis: list::get(string index)
 * Variables:
 *   (empty) - element of list at position index (starting from zero) at the time of initialization
 * 
 * Synopsis: list::shift()
 * 
 * Synopsis: list::contains(value)
 * Variables:
 *   (empty) - "true" if list contains value, "false" if not
 * 
 * Synopsis:
 *   list::find(start_pos, value)
 * Description:
 *   finds the first occurrence of 'value' in the list at position >='start_pos'.
 * Variables:
 *   pos - position of element, or "none" if not found
 *   found - "true" if found, "false" if not
 * 
 * Sysnopsis:
 *   list::remove_at(remove_pos)
 * Description:
 *   Removes the element at position 'remove_pos', which must refer to an existing element.
 * 
 * Synopsis:
 *   list::remove(value)
 * Description:
 *   Removes the first occurrence of value in the list, which must be in the list.
 * 
 * Synopsis:
 *   list::set(list l1, ..., list lN)
 * Description:
 *   Replaces the list with the concatenation of given lists.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <misc/offset.h>
#include <misc/parse_number.h>
#include <structure/IndexedList.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_list.h>

struct elem {
    IndexedListNode il_node;
    NCDValMem mem;
    NCDValRef val;
};

struct instance {
    NCDModuleInst *i;
    IndexedList il;
};

struct length_instance {
    NCDModuleInst *i;
    uint64_t length;
};

struct get_instance {
    NCDModuleInst *i;
    NCDValMem mem;
    NCDValRef val;
};

struct contains_instance {
    NCDModuleInst *i;
    int contains;
};

struct find_instance {
    NCDModuleInst *i;
    int is_found;
    uint64_t found_pos;
};

static uint64_t list_count (struct instance *o)
{
    return IndexedList_Count(&o->il);
}

static struct elem * insert_value (NCDModuleInst *i, struct instance *o, NCDValRef val, uint64_t idx)
{
    ASSERT(idx <= list_count(o))
    ASSERT(!NCDVal_IsInvalid(val))
    
    struct elem *e = malloc(sizeof(*e));
    if (!e) {
        ModuleLog(i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    NCDValMem_Init(&e->mem, i->params->iparams->string_index);
    
    e->val = NCDVal_NewCopy(&e->mem, val);
    if (NCDVal_IsInvalid(e->val)) {
        goto fail1;
    }
    
    IndexedList_InsertAt(&o->il, &e->il_node, idx);
    
    return e;
    
fail1:
    NCDValMem_Free(&e->mem);
    free(e);
fail0:
    return NULL;
}

static void remove_elem (struct instance *o, struct elem *e)
{
    IndexedList_Remove(&o->il, &e->il_node);
    NCDValMem_Free(&e->mem);
    free(e);
}

static struct elem * get_elem_at (struct instance *o, uint64_t idx)
{
    ASSERT(idx < list_count(o))
    
    IndexedListNode *iln = IndexedList_GetAt(&o->il, idx);
    struct elem *e = UPPER_OBJECT(iln, struct elem, il_node);
    
    return e;
}

static struct elem * get_first_elem (struct instance *o)
{
    ASSERT(list_count(o) > 0)
    
    IndexedListNode *iln = IndexedList_GetFirst(&o->il);
    struct elem *e = UPPER_OBJECT(iln, struct elem, il_node);
    
    return e;
}

static struct elem * get_last_elem (struct instance *o)
{
    ASSERT(list_count(o) > 0)
    
    IndexedListNode *iln = IndexedList_GetLast(&o->il);
    struct elem *e = UPPER_OBJECT(iln, struct elem, il_node);
    
    return e;
}

static void cut_list_front (struct instance *o, uint64_t count)
{
    while (list_count(o) > count) {
        remove_elem(o, get_first_elem(o));
    }
}

static void cut_list_back (struct instance *o, uint64_t count)
{
    while (list_count(o) > count) {
        remove_elem(o, get_last_elem(o));
    }
}

static int append_list_contents (NCDModuleInst *i, struct instance *o, NCDValRef args)
{
    ASSERT(NCDVal_IsList(args))
    
    uint64_t orig_count = list_count(o);
    
    size_t append_count = NCDVal_ListCount(args);
    
    for (size_t j = 0; j < append_count; j++) {
        NCDValRef elem = NCDVal_ListGet(args, j);
        if (!insert_value(i, o, elem, list_count(o))) {
            goto fail;
        }
    }
    
    return 1;
    
fail:
    cut_list_back(o, orig_count);
    return 0;
}

static int append_list_contents_contents (NCDModuleInst *i, struct instance *o, NCDValRef args)
{
    ASSERT(NCDVal_IsList(args))
    
    uint64_t orig_count = list_count(o);
    
    size_t append_count = NCDVal_ListCount(args);
    
    for (size_t j = 0; j < append_count; j++) {
        NCDValRef elem = NCDVal_ListGet(args, j);
        
        if (!NCDVal_IsList(elem)) {
            ModuleLog(i, BLOG_ERROR, "wrong type");
            goto fail;
        }
        
        if (!append_list_contents(i, o, elem)) {
            goto fail;
        }
    }
    
    return 1;
    
fail:
    cut_list_back(o, orig_count);
    return 0;
}

static struct elem * find_elem (struct instance *o, NCDValRef val, uint64_t start_idx, uint64_t *out_idx)
{
    if (start_idx >= list_count(o)) {
        return NULL;
    }
    
    for (IndexedListNode *iln = IndexedList_GetAt(&o->il, start_idx); iln; iln = IndexedList_GetNext(&o->il, iln)) {
        struct elem *e = UPPER_OBJECT(iln, struct elem, il_node);
        if (NCDVal_Compare(e->val, val) == 0) {
            if (out_idx) {
                *out_idx = start_idx;
            }
            return e;
        }
        start_idx++;
    }
    
    return NULL;
}

static int list_to_value (NCDModuleInst *i, struct instance *o, NCDValMem *mem, NCDValRef *out_val)
{
    *out_val = NCDVal_NewList(mem, IndexedList_Count(&o->il));
    if (NCDVal_IsInvalid(*out_val)) {
        goto fail;
    }
    
    for (IndexedListNode *iln = IndexedList_GetFirst(&o->il); iln; iln = IndexedList_GetNext(&o->il, iln)) {
        struct elem *e = UPPER_OBJECT(iln, struct elem, il_node);
        
        NCDValRef copy = NCDVal_NewCopy(mem, e->val);
        if (NCDVal_IsInvalid(copy)) {
            goto fail;
        }
        
        if (!NCDVal_ListAppend(*out_val, copy)) {
            goto fail;
        }
    }
    
    return 1;
    
fail:
    return 0;
}

static void func_new_list (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // init list
    IndexedList_Init(&o->il);
    
    // append contents
    if (!append_list_contents(i, o, params->args)) {
        goto fail1;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail1:
    cut_list_front(o, 0);
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_listfrom (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // init list
    IndexedList_Init(&o->il);
    
    // append contents contents
    if (!append_list_contents_contents(i, o, params->args)) {
        goto fail1;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail1:
    cut_list_front(o, 0);
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    // free list elements
    cut_list_front(o, 0);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (!strcmp(name, "")) {
        if (!list_to_value(o->i, o, mem, out)) {
            return 0;
        }
        
        return 1;
    }
    
    if (!strcmp(name, "length")) {
        *out = ncd_make_uintmax(mem, list_count(o));
        return 1;
    }
    
    return 0;
}

static void append_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    NCDValRef arg;
    if (!NCDVal_ListRead(params->args, 1, &arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // append
    if (!insert_value(i, mo, arg, list_count(mo))) {
        goto fail0;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void appendv_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    NCDValRef arg;
    if (!NCDVal_ListRead(params->args, 1, &arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsList(arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // append
    if (!append_list_contents(i, mo, arg)) {
        goto fail0;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void length_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct length_instance *o = vo;
    o->i = i;
    
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // remember length
    o->length = list_count(mo);
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void length_func_die (void *vo)
{
    struct length_instance *o = vo;
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int length_func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct length_instance *o = vo;
    
    if (!strcmp(name, "")) {
        *out = ncd_make_uintmax(mem, o->length);
        return 1;
    }
    
    return 0;
}
    
static void get_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct get_instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef index_arg;
    if (!NCDVal_ListRead(params->args, 1, &index_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    uintmax_t index;
    if (!ncd_read_uintmax(index_arg, &index)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong value");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // check index
    if (index >= list_count(mo)) {
        ModuleLog(o->i, BLOG_ERROR, "no element at index %"PRIuMAX, index);
        goto fail0;
    }
    
    // get element
    struct elem *e = get_elem_at(mo, index);
    
    // init mem
    NCDValMem_Init(&o->mem, i->params->iparams->string_index);
    
    // copy value
    o->val = NCDVal_NewCopy(&o->mem, e->val);
    if (NCDVal_IsInvalid(o->val)) {
        goto fail1;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail1:
    NCDValMem_Free(&o->mem);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void get_func_die (void *vo)
{
    struct get_instance *o = vo;
    
    // free mem
    NCDValMem_Free(&o->mem);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int get_func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct get_instance *o = vo;
    
    if (!strcmp(name, "")) {
        *out = NCDVal_NewCopy(mem, o->val);
        return 1;
    }
    
    return 0;
}

static void shift_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // check first
    if (list_count(mo) == 0) {
        ModuleLog(i, BLOG_ERROR, "list has no elements");
        goto fail0;
    }
    
    // remove first
    remove_elem(mo, get_first_elem(mo));
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void contains_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct contains_instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef value_arg;
    if (!NCDVal_ListRead(params->args, 1, &value_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // search
    o->contains = !!find_elem(mo, value_arg, 0, NULL);
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void contains_func_die (void *vo)
{
    struct contains_instance *o = vo;
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int contains_func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct contains_instance *o = vo;
    
    if (!strcmp(name, "")) {
        *out = ncd_make_boolean(mem, o->contains);
        return 1;
    }
    
    return 0;
}

static void find_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct find_instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef start_pos_arg;
    NCDValRef value_arg;
    if (!NCDVal_ListRead(params->args, 2, &start_pos_arg, &value_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // read start position
    uintmax_t start_pos;
    if (!ncd_read_uintmax(start_pos_arg, &start_pos) || start_pos > UINT64_MAX) {
        ModuleLog(o->i, BLOG_ERROR, "wrong start pos");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // find
    o->is_found = !!find_elem(mo, value_arg, start_pos, &o->found_pos);
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void find_func_die (void *vo)
{
    struct find_instance *o = vo;
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int find_func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct find_instance *o = vo;
    
    if (!strcmp(name, "pos")) {
        char value[64] = "none";
        
        if (o->is_found) {
            generate_decimal_repr_string(o->found_pos, value);
        }
        
        *out = NCDVal_NewString(mem, value);
        return 1;
    }
    
    if (!strcmp(name, "found")) {
        *out = ncd_make_boolean(mem, o->is_found);
        return 1;
    }
    
    return 0;
}

static void removeat_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // read arguments
    NCDValRef remove_pos_arg;
    if (!NCDVal_ListRead(params->args, 1, &remove_pos_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // read position
    uintmax_t remove_pos;
    if (!ncd_read_uintmax(remove_pos_arg, &remove_pos)) {
        ModuleLog(i, BLOG_ERROR, "wrong pos");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // check position
    if (remove_pos >= list_count(mo)) {
        ModuleLog(i, BLOG_ERROR, "pos out of range");
        goto fail0;
    }
    
    // remove
    remove_elem(mo, get_elem_at(mo, remove_pos));
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void remove_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // read arguments
    NCDValRef value_arg;
    if (!NCDVal_ListRead(params->args, 1, &value_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // find element
    struct elem *e = find_elem(mo, value_arg, 0, NULL);
    if (!e) {
        ModuleLog(i, BLOG_ERROR, "value does not exist");
        goto fail0;
    }
    
    // remove element
    remove_elem(mo, e);
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void set_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // remember old count
    uint64_t old_count = list_count(mo);
    
    // append contents of our lists
    if (!append_list_contents_contents(i, mo, params->args)) {
        goto fail0;
    }
    
    // remove old elements
    cut_list_front(mo, list_count(mo) - old_count);
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "list",
        .func_new2 = func_new_list,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "listfrom",
        .base_type = "list",
        .func_new2 = func_new_listfrom,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "concatlist", // alias for listfrom
        .base_type = "list",
        .func_new2 = func_new_listfrom,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "list::append",
        .func_new2 = append_func_new
    }, {
        .type = "list::appendv",
        .func_new2 = appendv_func_new
    }, {
        .type = "list::length",
        .func_new2 = length_func_new,
        .func_die = length_func_die,
        .func_getvar = length_func_getvar,
        .alloc_size = sizeof(struct length_instance)
    }, {
        .type = "list::get",
        .func_new2 = get_func_new,
        .func_die = get_func_die,
        .func_getvar = get_func_getvar,
        .alloc_size = sizeof(struct get_instance)
    }, {
        .type = "list::shift",
        .func_new2 = shift_func_new
    }, {
        .type = "list::contains",
        .func_new2 = contains_func_new,
        .func_die = contains_func_die,
        .func_getvar = contains_func_getvar,
        .alloc_size = sizeof(struct contains_instance)
    }, {
        .type = "list::find",
        .func_new2 = find_func_new,
        .func_die = find_func_die,
        .func_getvar = find_func_getvar,
        .alloc_size = sizeof(struct find_instance)
    }, {
        .type = "list::remove_at",
        .func_new2 = removeat_func_new
    }, {
        .type = "list::remove",
        .func_new2 = remove_func_new
    }, {
        .type = "list::set",
        .func_new2 = set_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_list = {
    .modules = modules
};
