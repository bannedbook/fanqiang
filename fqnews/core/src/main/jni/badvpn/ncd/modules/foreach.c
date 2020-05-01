/**
 * @file foreach.c
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
 *   foreach(list/map collection, string template, list args)
 * 
 * Description:
 *   Initializes a template process for each element of list, sequentially,
 *   obeying to the usual execution model of NCD.
 *   It's equivalent to (except for special variables):
 * 
 *   call(template, args);
 *   ...
 *   call(template, args); # one call() for every element of list
 * 
 * Template process specials:
 * 
 *   _index - (lists only) index of the list element corresponding to the template,
 *            process, as a decimal string, starting from zero
 *   _elem - (lists only) element of list corresponding to the template process
 *   _key - (maps only) key of the current map entry
 *   _val - (maps only) value of the current map entry
 *   _caller.X - X as seen from the foreach() statement
 * 
 * Synopsis:
 *   foreach_emb(list/map collection, string template, string name1 [, string name2])
 * 
 * Description:
 *   Foreach for embedded templates; the desugaring process converts Foreach
 *   clauses into this statement. The called templates have direct access to
 *   objects as seen from this statement, and also some kind of access to the
 *   current element of the iteration, depending on the type of collection
 *   being iterated, and whether 'name2' is provided:
 *   List and one name: current element is named 'name1'.
 *   List and both names: current index is named 'name1', current element 'name2'.
 *   Map and one name: current key is named 'name1'.
 *   Map and both names: current key is named 'name1', current value 'name2'.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <misc/balloc.h>
#include <misc/string_begins_with.h>
#include <misc/debug.h>
#include <misc/offset.h>
#include <system/BReactor.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_foreach.h>

#define ISTATE_WORKING 1
#define ISTATE_UP 2
#define ISTATE_WAITING 3
#define ISTATE_TERMINATING 4

#define ESTATE_FORGOTTEN 1
#define ESTATE_DOWN 2
#define ESTATE_UP 3
#define ESTATE_WAITING 4
#define ESTATE_TERMINATING 5

struct element;

struct instance {
    NCDModuleInst *i;
    NCDValRef template_name;
    NCDValRef args;
    NCD_string_id_t name1;
    NCD_string_id_t name2;
    BTimer timer;
    struct element *elems;
    int type;
    int num_elems;
    int gp; // good pointer
    int ip; // initialized pointer
    int state;
};

struct element {
    struct instance *inst;
    union {
        struct {
            NCDValRef list_elem;
        };
        struct {
            NCDValRef map_key;
            NCDValRef map_val;
        };
    };
    NCDModuleProcess process;
    int i;
    int state;
};

static void assert_state (struct instance *o);
static void work (struct instance *o);
static void advance (struct instance *o);
static void timer_handler (struct instance *o);
static void element_process_handler_event (NCDModuleProcess *process, int event);
static int element_process_func_getspecialobj (NCDModuleProcess *process, NCD_string_id_t name, NCDObject *out_object);
static int element_caller_object_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object);
static int element_list_index_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out);
static int element_list_elem_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out);
static int element_map_key_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out);
static int element_map_val_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out);
static void instance_free (struct instance *o);

enum {STRING_INDEX, STRING_ELEM, STRING_KEY, STRING_VAL};

static const char *strings[] = {
    "_index", "_elem", "_key", "_val", NULL
};

static void assert_state (struct instance *o)
{
    ASSERT(o->num_elems >= 0)
    ASSERT(o->gp >= 0)
    ASSERT(o->ip >= 0)
    ASSERT(o->gp <= o->num_elems)
    ASSERT(o->ip <= o->num_elems)
    ASSERT(o->gp <= o->ip)
    
#ifndef NDEBUG
    // check GP
    for (int i = 0; i < o->gp; i++) {
        if (i == o->gp - 1) {
            ASSERT(o->elems[i].state == ESTATE_UP || o->elems[i].state == ESTATE_DOWN ||
                   o->elems[i].state == ESTATE_WAITING)
        } else {
            ASSERT(o->elems[i].state == ESTATE_UP)
        }
    }
    
    // check IP
    int ip = o->num_elems;
    while (ip > 0 && o->elems[ip - 1].state == ESTATE_FORGOTTEN) {
        ip--;
    }
    ASSERT(o->ip == ip)
    
    // check gap
    for (int i = o->gp; i < o->ip; i++) {
        if (i == o->ip - 1) {
            ASSERT(o->elems[i].state == ESTATE_UP || o->elems[i].state == ESTATE_DOWN ||
                   o->elems[i].state == ESTATE_WAITING || o->elems[i].state == ESTATE_TERMINATING)
        } else {
            ASSERT(o->elems[i].state == ESTATE_UP || o->elems[i].state == ESTATE_DOWN ||
                   o->elems[i].state == ESTATE_WAITING)
        }
    }
#endif
}

static void work (struct instance *o)
{
    assert_state(o);
    
    // stop timer
    BReactor_RemoveTimer(o->i->params->iparams->reactor, &o->timer);
    
    if (o->state == ISTATE_WAITING) {
        return;
    }
    
    if (o->state == ISTATE_UP && !(o->gp == o->ip && o->gp == o->num_elems && (o->gp == 0 || o->elems[o->gp - 1].state == ESTATE_UP))) {
        // signal down
        NCDModuleInst_Backend_Down(o->i);
        
        // set state waiting
        o->state = ISTATE_WAITING;
        return;
    }
    
    if (o->gp < o->ip) {
        // get last element
        struct element *le = &o->elems[o->ip - 1];
        ASSERT(le->state != ESTATE_FORGOTTEN)
        
        // start terminating if not already
        if (le->state != ESTATE_TERMINATING) {
            // request termination
            NCDModuleProcess_Terminate(&le->process);
            
            // set element state terminating
            le->state = ESTATE_TERMINATING;
        }
        
        return;
    }
    
    if (o->state == ISTATE_TERMINATING) {
        // finally die
        instance_free(o);
        return;
    }
    
    if (o->gp == o->num_elems && (o->gp == 0 || o->elems[o->gp - 1].state == ESTATE_UP)) {
        if (o->state == ISTATE_WORKING) {
            // signal up
            NCDModuleInst_Backend_Up(o->i);
            
            // set state up
            o->state = ISTATE_UP;
        }
        
        return;
    }
    
    if (o->gp > 0 && o->elems[o->gp - 1].state == ESTATE_WAITING) {
        // get last element
        struct element *le = &o->elems[o->gp - 1];
        
        // continue process
        NCDModuleProcess_Continue(&le->process);
        
        // set state down
        le->state = ESTATE_DOWN;
        return;
    }
    
    if (o->gp > 0 && o->elems[o->gp - 1].state == ESTATE_DOWN) {
        return;
    }
    
    ASSERT(o->gp == 0 || o->elems[o->gp - 1].state == ESTATE_UP)
    
    advance(o);
    return;
}

static void advance (struct instance *o)
{
    assert_state(o);
    ASSERT(o->gp == o->ip)
    ASSERT(o->gp < o->num_elems)
    ASSERT(o->gp == 0 || o->elems[o->gp - 1].state == ESTATE_UP)
    ASSERT(o->elems[o->gp].state == ESTATE_FORGOTTEN)
    
    // get next element
    struct element *e = &o->elems[o->gp];
    
    // init process
    if (!NCDModuleProcess_InitValue(&e->process, o->i, o->template_name, o->args, element_process_handler_event)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDModuleProcess_Init failed");
        goto fail;
    }
    
    // set special functions
    NCDModuleProcess_SetSpecialFuncs(&e->process, element_process_func_getspecialobj);
    
    // set element state down
    e->state = ESTATE_DOWN;
    
    // increment GP and IP
    o->gp++;
    o->ip++;
    return;
    
fail:
    // set timer
    BReactor_SetTimer(o->i->params->iparams->reactor, &o->timer);
}

static void timer_handler (struct instance *o)
{
    assert_state(o);
    ASSERT(o->gp == o->ip)
    ASSERT(o->gp < o->num_elems)
    ASSERT(o->gp == 0 || o->elems[o->gp - 1].state == ESTATE_UP)
    ASSERT(o->elems[o->gp].state == ESTATE_FORGOTTEN)
    
    advance(o);
    return;
}

static void element_process_handler_event (NCDModuleProcess *process, int event)
{
    struct element *e = UPPER_OBJECT(process, struct element, process);
    struct instance *o = e->inst;
    assert_state(o);
    ASSERT(e->i < o->ip)
    ASSERT(e->state != ESTATE_FORGOTTEN)
    
    switch (event) {
        case NCDMODULEPROCESS_EVENT_UP: {
            ASSERT(e->state == ESTATE_DOWN)
            ASSERT(o->gp == o->ip)
            ASSERT(o->gp == e->i + 1)
            
            // set element state up
            e->state = ESTATE_UP;
        } break;
        
        case NCDMODULEPROCESS_EVENT_DOWN: {
            ASSERT(e->state == ESTATE_UP)
            
            // set element state waiting
            e->state = ESTATE_WAITING;
            
            // bump down GP
            if (o->gp > e->i + 1) {
                o->gp = e->i + 1;
            }
        } break;
        
        case NCDMODULEPROCESS_EVENT_TERMINATED: {
            ASSERT(e->state == ESTATE_TERMINATING)
            ASSERT(o->gp < o->ip)
            ASSERT(o->ip == e->i + 1)
            
            // free process
            NCDModuleProcess_Free(&e->process);
            
            // set element state forgotten
            e->state = ESTATE_FORGOTTEN;
            
            // decrement IP
            o->ip--;
        } break;
        
        default: ASSERT(0);
    }
    
    work(o);
    return;
}

static int element_process_func_getspecialobj (NCDModuleProcess *process, NCD_string_id_t name, NCDObject *out_object)
{
    struct element *e = UPPER_OBJECT(process, struct element, process);
    struct instance *o = e->inst;
    ASSERT(e->state != ESTATE_FORGOTTEN)
    
    switch (o->type) {
        case NCDVAL_LIST: {
            NCD_string_id_t index_name = (o->name2 >= 0 ? o->name1 : -1);
            NCD_string_id_t elem_name = (o->name2 >= 0 ? o->name2 : o->name1);
            
            if (index_name >= 0 && name == index_name) {
                *out_object = NCDObject_Build(-1, e, element_list_index_object_func_getvar, NCDObject_no_getobj);
                return 1;
            }
            
            if (name == elem_name) {
                *out_object = NCDObject_Build(-1, e, element_list_elem_object_func_getvar, NCDObject_no_getobj);
                return 1;
            }
        } break;
        case NCDVAL_MAP: {
            NCD_string_id_t key_name = o->name1;
            NCD_string_id_t val_name = o->name2;
            
            if (name == key_name) {
                *out_object = NCDObject_Build(-1, e, element_map_key_object_func_getvar, NCDObject_no_getobj);
                return 1;
            }
            
            if (val_name >= 0 && name == val_name) {
                *out_object = NCDObject_Build(-1, e, element_map_val_object_func_getvar, NCDObject_no_getobj);
                return 1;
            }
        } break;
    }
    
    if (NCDVal_IsInvalid(o->args)) {
        return NCDModuleInst_Backend_GetObj(o->i, name, out_object);
    }
    
    if (name == NCD_STRING_CALLER) {
        *out_object = NCDObject_Build(-1, e, NCDObject_no_getvar, element_caller_object_func_getobj);
        return 1;
    }
    
    return 0;
}

static int element_caller_object_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object)
{
    struct element *e = NCDObject_DataPtr(obj);
    struct instance *o = e->inst;
    ASSERT(e->state != ESTATE_FORGOTTEN)
    ASSERT(!NCDVal_IsInvalid(o->args))
    
    return NCDModuleInst_Backend_GetObj(o->i, name, out_object);
}

static int element_list_index_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct element *e = NCDObject_DataPtr(obj);
    struct instance *o = e->inst;
    B_USE(o)
    ASSERT(e->state != ESTATE_FORGOTTEN)
    ASSERT(o->type == NCDVAL_LIST)
    
    if (name != NCD_STRING_EMPTY) {
        return 0;
    }
    
    *out = ncd_make_uintmax(mem, e->i);
    return 1;
}

static int element_list_elem_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct element *e = NCDObject_DataPtr(obj);
    struct instance *o = e->inst;
    B_USE(o)
    ASSERT(e->state != ESTATE_FORGOTTEN)
    ASSERT(o->type == NCDVAL_LIST)
    
    if (name != NCD_STRING_EMPTY) {
        return 0;
    }
    
    *out = NCDVal_NewCopy(mem, e->list_elem);
    return 1;
}

static int element_map_key_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct element *e = NCDObject_DataPtr(obj);
    struct instance *o = e->inst;
    B_USE(o)
    ASSERT(e->state != ESTATE_FORGOTTEN)
    ASSERT(o->type == NCDVAL_MAP)
    
    if (name != NCD_STRING_EMPTY) {
        return 0;
    }
    
    *out = NCDVal_NewCopy(mem, e->map_key);
    return 1;
}

static int element_map_val_object_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct element *e = NCDObject_DataPtr(obj);
    struct instance *o = e->inst;
    B_USE(o)
    ASSERT(e->state != ESTATE_FORGOTTEN)
    ASSERT(o->type == NCDVAL_MAP)
    
    if (name != NCD_STRING_EMPTY) {
        return 0;
    }
    
    *out = NCDVal_NewCopy(mem, e->map_val);
    return 1;
}

static void func_new_common (void *vo, NCDModuleInst *i, NCDValRef collection, NCDValRef template_name, NCDValRef args, NCD_string_id_t name1, NCD_string_id_t name2)
{
    ASSERT(!NCDVal_IsInvalid(collection))
    ASSERT(NCDVal_IsString(template_name))
    ASSERT(NCDVal_IsInvalid(args) || NCDVal_IsList(args))
    ASSERT(name1 >= 0)
    
    struct instance *o = vo;
    o->i = i;
    
    o->type = NCDVal_Type(collection);
    o->template_name = template_name;
    o->args = args;
    o->name1 = name1;
    o->name2 = name2;
    
    // init timer
    btime_t retry_time = NCDModuleInst_Backend_InterpGetRetryTime(i);
    BTimer_Init(&o->timer, retry_time, (BTimer_handler)timer_handler, o);
    
    size_t num_elems;
    NCDValMapElem cur_map_elem;
    
    switch (o->type) {
        case NCDVAL_LIST: {
            num_elems = NCDVal_ListCount(collection);
        } break;
        case NCDVAL_MAP: {
            num_elems = NCDVal_MapCount(collection);
            cur_map_elem = NCDVal_MapOrderedFirst(collection); 
        } break;
        default:
            ModuleLog(i, BLOG_ERROR, "invalid collection type");
            goto fail0;
    }
    
    if (num_elems > INT_MAX) {
        ModuleLog(i, BLOG_ERROR, "too many elements");
        goto fail0;
    }
    o->num_elems = num_elems;
    
    // allocate elements
    if (!(o->elems = BAllocArray(o->num_elems, sizeof(o->elems[0])))) {
        ModuleLog(i, BLOG_ERROR, "BAllocArray failed");
        goto fail0;
    }
    
    for (int j = 0; j < o->num_elems; j++) {
        struct element *e = &o->elems[j];
        
        // set instance
        e->inst = o;
        
        // set index
        e->i = j;
        
        // set state forgotten
        e->state = ESTATE_FORGOTTEN;
        
        // set values
        switch (o->type) {
            case NCDVAL_LIST: {
                e->list_elem = NCDVal_ListGet(collection, j);
            } break;
            case NCDVAL_MAP: {
                e->map_key = NCDVal_MapElemKey(collection, cur_map_elem);
                e->map_val = NCDVal_MapElemVal(collection, cur_map_elem);
                cur_map_elem = NCDVal_MapOrderedNext(collection, cur_map_elem);
            } break;
        }
    }
    
    // set GP and IP zero
    o->gp = 0;
    o->ip = 0;
    
    // set state working
    o->state = ISTATE_WORKING;
    
    work(o);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_foreach (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // read arguments
    NCDValRef arg_collection;
    NCDValRef arg_template;
    NCDValRef arg_args;
    if (!NCDVal_ListRead(params->args, 3, &arg_collection, &arg_template, &arg_args)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(arg_template) || !NCDVal_IsList(arg_args)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    NCD_string_id_t name1;
    NCD_string_id_t name2;
    
    switch (NCDVal_Type(arg_collection)) {
        case NCDVAL_LIST: {
            name1 = ModuleString(i, STRING_INDEX);
            name2 = ModuleString(i, STRING_ELEM);
        } break;
        case NCDVAL_MAP: {
            name1 = ModuleString(i, STRING_KEY);
            name2 = ModuleString(i, STRING_VAL);
        } break;
        default:
            ModuleLog(i, BLOG_ERROR, "invalid collection type");
            goto fail0;
    }
    
    func_new_common(vo, i, arg_collection, arg_template, arg_args, name1, name2);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_foreach_emb (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // read arguments
    NCDValRef arg_collection;
    NCDValRef arg_template;
    NCDValRef arg_name1;
    NCDValRef arg_name2 = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 3, &arg_collection, &arg_template, &arg_name1) && !NCDVal_ListRead(params->args, 4, &arg_collection, &arg_template, &arg_name1, &arg_name2)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(arg_template) || !NCDVal_IsString(arg_name1) || (!NCDVal_IsInvalid(arg_name2) && !NCDVal_IsString(arg_name2))) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    NCD_string_id_t name1 = ncd_get_string_id(arg_name1);
    if (name1 < 0) {
        ModuleLog(i, BLOG_ERROR, "ncd_get_string_id failed");
        goto fail0;
    }
    
    NCD_string_id_t name2 = -1;
    if (!NCDVal_IsInvalid(arg_name2)) {
        name2 = ncd_get_string_id(arg_name2);
        if (name2 < 0) {
            ModuleLog(i, BLOG_ERROR, "ncd_get_string_id failed");
            goto fail0;
        }
    }
    
    func_new_common(vo, i, arg_collection, arg_template, NCDVal_NewInvalid(), name1, name2);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void instance_free (struct instance *o)
{
    ASSERT(o->gp == 0)
    ASSERT(o->ip == 0)
    
    // free elements
    BFree(o->elems);
    
    // free timer
    BReactor_RemoveTimer(o->i->params->iparams->reactor, &o->timer);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    assert_state(o);
    ASSERT(o->state != ISTATE_TERMINATING)
    
    // set GP zero
    o->gp = 0;
    
    // set state terminating
    o->state = ISTATE_TERMINATING;
    
    work(o);
    return;
}

static void func_clean (void *vo)
{
    struct instance *o = vo;
    
    if (o->state != ISTATE_WAITING) {
        return;
    }
    
    // set state working
    o->state = ISTATE_WORKING;
    
    work(o);
    return;
}

static struct NCDModule modules[] = {
    {
        .type = "foreach",
        .func_new2 = func_new_foreach,
        .func_die = func_die,
        .func_clean = func_clean,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "foreach_emb",
        .func_new2 = func_new_foreach_emb,
        .func_die = func_die,
        .func_clean = func_clean,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_foreach = {
    .modules = modules,
    .strings = strings
};
