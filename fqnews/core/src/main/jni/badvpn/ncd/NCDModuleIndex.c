/**
 * @file NCDModuleIndex.c
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

#include <misc/offset.h>
#include <misc/balloc.h>
#include <misc/bsize.h>
#include <misc/hashfun.h>
#include <misc/compare.h>
#include <misc/substring.h>
#include <base/BLog.h>

#include "NCDModuleIndex.h"

#include <generated/blog_channel_NCDModuleIndex.h>

#include "NCDModuleIndex_mhash.h"
#include <structure/CHash_impl.h>

#include "NCDModuleIndex_func_vec.h"
#include <structure/Vector_impl.h>

#include "NCDModuleIndex_fhash.h"
#include <structure/CHash_impl.h>

static int string_pointer_comparator (void *user, void *v1, void *v2)
{
    const char **s1 = v1;
    const char **s2 = v2;
    int cmp = strcmp(*s1, *s2);
    return B_COMPARE(cmp, 0);
}

static struct NCDModuleIndex_module * find_module (NCDModuleIndex *o, const char *type)
{
    NCDModuleIndex__MHashRef ref = NCDModuleIndex__MHash_Lookup(&o->modules_hash, 0, type);
    ASSERT(!ref.link || !strcmp(ref.link->imodule.module.type, type))
    return ref.link;
}

#ifndef NDEBUG
static struct NCDModuleIndex_base_type * find_base_type (NCDModuleIndex *o, const char *base_type)
{
    BAVLNode *node = BAVL_LookupExact(&o->base_types_tree, &base_type);
    if (!node) {
        return NULL;
    }
    
    struct NCDModuleIndex_base_type *bt = UPPER_OBJECT(node, struct NCDModuleIndex_base_type, base_types_tree_node);
    ASSERT(!strcmp(bt->base_type, base_type))
    
    return bt;
}
#endif

static int add_method (const char *type, const struct NCDInterpModule *module, NCDMethodIndex *method_index, int *out_method_id)
{
    ASSERT(type)
    ASSERT(module)
    ASSERT(method_index)
    ASSERT(out_method_id)
    
    const char search[] = "::";
    size_t search_len = sizeof(search) - 1;
    
    size_t table[sizeof(search) - 1];
    build_substring_backtrack_table_reverse(MemRef_Make(search, search_len), table);
    
    size_t pos;
    if (!find_substring_reverse(MemRef_MakeCstr(type), MemRef_Make(search, search_len), table, &pos)) {
        *out_method_id = -1;
        return 1;
    }
    
    ASSERT(pos >= 0)
    ASSERT(pos <= strlen(type) - search_len)
    ASSERT(!memcmp(type + pos, search, search_len))
    
    int method_id = NCDMethodIndex_AddMethod(method_index, type, pos, type + pos + search_len, module);
    if (method_id < 0) {
        BLog(BLOG_ERROR, "NCDMethodIndex_AddMethod failed");
        return 0;
    }
    
    *out_method_id = method_id;
    return 1;
}

int NCDModuleIndex_Init (NCDModuleIndex *o, NCDStringIndex *string_index)
{
    ASSERT(string_index)
    
    // init modules hash
    if (!NCDModuleIndex__MHash_Init(&o->modules_hash, NCDMODULEINDEX_MODULES_HASH_SIZE)) {
        BLog(BLOG_ERROR, "NCDModuleIndex__MHash_Init failed");
        goto fail0;
    }
    
#ifndef NDEBUG
    // init base types tree
    BAVL_Init(&o->base_types_tree, OFFSET_DIFF(struct NCDModuleIndex_base_type, base_type, base_types_tree_node), string_pointer_comparator, NULL);
#endif
    
    // init groups list
    LinkedList0_Init(&o->groups_list);
    
    // init method index
    if (!NCDMethodIndex_Init(&o->method_index, string_index)) {
        BLog(BLOG_ERROR, "NCDMethodIndex_Init failed");
        goto fail1;
    }
    
    // init functions vector
    if (!NCDModuleIndex__FuncVec_Init(&o->func_vec, NCDMODULEINDEX_FUNCTIONS_VEC_INITIAL_SIZE)) {
        BLog(BLOG_ERROR, "NCDModuleIndex__FuncVec_Init failed");
        goto fail2;
    }
    
    // init functions hash
    if (!NCDModuleIndex__FuncHash_Init(&o->func_hash, NCDMODULEINDEX_FUNCTIONS_HASH_SIZE)) {
        BLog(BLOG_ERROR, "NCDModuleIndex__FuncHash_Init failed");
        goto fail3;
    }
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail3:
    NCDModuleIndex__FuncVec_Free(&o->func_vec);
fail2:
    NCDMethodIndex_Free(&o->method_index);
fail1:
    NCDModuleIndex__MHash_Free(&o->modules_hash);
fail0:
    return 0;
}

void NCDModuleIndex_Free (NCDModuleIndex *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free groups
    LinkedList0Node *ln;
    while (ln = LinkedList0_GetFirst(&o->groups_list)) {
        struct NCDModuleIndex_group *ig = UPPER_OBJECT(ln, struct NCDModuleIndex_group, groups_list_node);
        if (ig->igroup.group.func_globalfree) {
            ig->igroup.group.func_globalfree(&ig->igroup);
        }
        BFree(ig->igroup.strings);
        LinkedList0_Remove(&o->groups_list, &ig->groups_list_node);
        BFree(ig);
    }
    
#ifndef NDEBUG
    // free base types
    BAVLNode *tn;
    while (tn = BAVL_GetFirst(&o->base_types_tree)) {
        struct NCDModuleIndex_base_type *bt = UPPER_OBJECT(tn, struct NCDModuleIndex_base_type, base_types_tree_node);
        BAVL_Remove(&o->base_types_tree, &bt->base_types_tree_node);
        BFree(bt);
    }
#endif
    
    // free functions hash
    NCDModuleIndex__FuncHash_Free(&o->func_hash);
    
    // free functions vector
    NCDModuleIndex__FuncVec_Free(&o->func_vec);
    
    // free method index
    NCDMethodIndex_Free(&o->method_index);
    
    // free modules hash
    NCDModuleIndex__MHash_Free(&o->modules_hash);
}

int NCDModuleIndex_AddGroup (NCDModuleIndex *o, const struct NCDModuleGroup *group, const struct NCDModuleInst_iparams *iparams, NCDStringIndex *string_index)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(group)
    ASSERT(iparams)
    ASSERT(string_index)
    
    // count modules in the group
    size_t num_modules = 0;
    if (group->modules) {
        while (group->modules[num_modules].type) {
            num_modules++;
        }
    }
    
    // compute allocation size
    bsize_t size = bsize_add(bsize_fromsize(sizeof(struct NCDModuleIndex_group)), bsize_mul(bsize_fromsize(num_modules), bsize_fromsize(sizeof(struct NCDModuleIndex_module))));
    
    // allocate group 
    struct NCDModuleIndex_group *ig = BAllocSize(size);
    if (!ig) {
        BLog(BLOG_ERROR, "BAllocSize failed");
        goto fail0;
    }
    
    // insert to groups list
    LinkedList0_Prepend(&o->groups_list, &ig->groups_list_node);
    
    // copy NCDModuleGroup
    ig->igroup.group = *group;
    
    if (!group->strings) {
        // not resolving strings
        ig->igroup.strings = NULL;
    } else {
        // compute number of strings
        size_t num_strings = 0;
        while (group->strings[num_strings]) {
            num_strings++;
        }
        
        // allocate array for string IDs
        ig->igroup.strings = BAllocArray(num_strings, sizeof(ig->igroup.strings[0]));
        if (!ig->igroup.strings) {
            BLog(BLOG_ERROR, "BAllocArray failed");
            goto fail1;
        }
        
        // map strings to IDs
        for (size_t i = 0; i < num_strings; i++) {
            ig->igroup.strings[i] = NCDStringIndex_Get(string_index, group->strings[i]);
            if (ig->igroup.strings[i] < 0) {
                BLog(BLOG_ERROR, "NCDStringIndex_Get failed");
                goto fail2;
            }
        }
    }
    
    // call group init function
    if (group->func_globalinit) {
        if (!group->func_globalinit(&ig->igroup, iparams)) {
            BLog(BLOG_ERROR, "func_globalinit failed");
            goto fail2;
        }
    }
    
    size_t num_inited_modules = 0;
    
    // initialize modules
    if (group->modules) {
        for (size_t i = 0; i < num_modules; i++) {
            const struct NCDModule *nm = &group->modules[i];
            struct NCDModuleIndex_module *m = &ig->modules[i];
            
            // make sure a module with this name doesn't exist already
            if (find_module(o, nm->type)) {
                BLog(BLOG_ERROR, "module type '%s' already exists", nm->type);
                goto loop_fail0;
            }
            
            // copy NCDModule structure
            m->imodule.module = *nm;
            
            // determine base type
            const char *base_type = (nm->base_type ? nm->base_type : nm->type);
            ASSERT(base_type)
            
            // map base type to ID
            m->imodule.base_type_id = NCDStringIndex_Get(string_index, base_type);
            if (m->imodule.base_type_id < 0) {
                BLog(BLOG_ERROR, "NCDStringIndex_Get failed");
                goto loop_fail0;
            }
            
            // set group pointer
            m->imodule.group = &ig->igroup;
            
            // register method
            if (!add_method(nm->type, &m->imodule, &o->method_index, &m->method_id)) {
                goto loop_fail0;
            }
            
#ifndef NDEBUG
            // ensure that this base_type does not appear in any other groups
            struct NCDModuleIndex_base_type *bt = find_base_type(o, base_type);
            if (bt) {
                if (bt->group != ig) {
                    BLog(BLOG_ERROR, "module base type '%s' already exists in another module group", base_type);
                    goto loop_fail1;
                }
            } else {
                if (!(bt = BAlloc(sizeof(*bt)))) {
                    BLog(BLOG_ERROR, "BAlloc failed");
                    goto loop_fail1;
                }
                bt->base_type = base_type;
                bt->group = ig;
                ASSERT_EXECUTE(BAVL_Insert(&o->base_types_tree, &bt->base_types_tree_node, NULL))
            }
#endif

            // insert to modules hash
            NCDModuleIndex__MHashRef ref = {m, m};
            int res = NCDModuleIndex__MHash_Insert(&o->modules_hash, 0, ref, NULL);
            ASSERT_EXECUTE(res)
            
            num_inited_modules++;
            continue;
            
#ifndef NDEBUG
        loop_fail1:
            if (m->method_id >= 0) {
                NCDMethodIndex_RemoveMethod(&o->method_index, m->method_id);
            }
#endif
        loop_fail0:
            goto fail3;
        }
    }
    
    size_t prev_func_count = NCDModuleIndex__FuncVec_Count(&o->func_vec);
    
    if (group->functions) {
        for (struct NCDModuleFunction const *mfunc = group->functions; mfunc->func_name; mfunc++) {
            NCD_string_id_t func_name_id = NCDStringIndex_Get(string_index, mfunc->func_name);
            if (func_name_id < 0) {
                BLog(BLOG_ERROR, "NCDStringIndex_Get failed");
                goto fail4;
            }
            
            NCDModuleIndex__FuncHashRef lookup_ref = NCDModuleIndex__FuncHash_Lookup(&o->func_hash, o, func_name_id);
            if (lookup_ref.link >= 0) {
                BLog(BLOG_ERROR, "Function already exists: %s", mfunc->func_name);
                goto fail4;
            }
            
            size_t func_index;
            struct NCDModuleIndex__Func *func = NCDModuleIndex__FuncVec_Push(&o->func_vec, &func_index);
            if (!func) {
                BLog(BLOG_ERROR, "NCDModuleIndex__FuncVec_Push failed");
                goto fail4;
            }
            
            func->ifunc.function = *mfunc;
            func->ifunc.func_name_id = func_name_id;
            func->ifunc.group = &ig->igroup;
            
            NCDModuleIndex__FuncHashRef ref = {func, func_index};
            int res = NCDModuleIndex__FuncHash_Insert(&o->func_hash, o, ref, NULL);
            ASSERT_EXECUTE(res)
        }
    }
    
    return 1;
    
fail4:
    while (NCDModuleIndex__FuncVec_Count(&o->func_vec) > prev_func_count) {
        size_t func_index;
        struct NCDModuleIndex__Func *func = NCDModuleIndex__FuncVec_Pop(&o->func_vec, &func_index);
        NCDModuleIndex__FuncHashRef ref = {func, func_index};
        NCDModuleIndex__FuncHash_Remove(&o->func_hash, o, ref);
    }
fail3:
    while (num_inited_modules-- > 0) {
        struct NCDModuleIndex_module *m = &ig->modules[num_inited_modules];
        NCDModuleIndex__MHashRef ref = {m, m};
        NCDModuleIndex__MHash_Remove(&o->modules_hash, 0, ref);
#ifndef NDEBUG
        const struct NCDModule *nm = &group->modules[num_inited_modules];
        const char *base_type = (nm->base_type ? nm->base_type : nm->type);
        struct NCDModuleIndex_base_type *bt = find_base_type(o, base_type);
        if (bt) {
            ASSERT(bt->group == ig)
            BAVL_Remove(&o->base_types_tree, &bt->base_types_tree_node);
            BFree(bt);
        }
#endif
        if (m->method_id >= 0) {
            NCDMethodIndex_RemoveMethod(&o->method_index, m->method_id);
        }
    }
    if (group->func_globalfree) {
        group->func_globalfree(&ig->igroup);
    }
fail2:
    BFree(ig->igroup.strings);
fail1:
    LinkedList0_Remove(&o->groups_list, &ig->groups_list_node);
    BFree(ig);
fail0:
    return 0;
}

const struct NCDInterpModule * NCDModuleIndex_FindModule (NCDModuleIndex *o, const char *type)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(type)
    
    struct NCDModuleIndex_module *m = find_module(o, type);
    if (!m) {
        return NULL;
    }
    
    return &m->imodule;
}

int NCDModuleIndex_GetMethodNameId (NCDModuleIndex *o, const char *method_name)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(method_name)
    
    return NCDMethodIndex_GetMethodNameId(&o->method_index, method_name);
}

const struct NCDInterpModule * NCDModuleIndex_GetMethodModule (NCDModuleIndex *o, NCD_string_id_t obj_type, int method_name_id)
{
    DebugObject_Access(&o->d_obj);
    
    return NCDMethodIndex_GetMethodModule(&o->method_index, obj_type, method_name_id);
}

const struct NCDInterpFunction * NCDModuleIndex_FindFunction (NCDModuleIndex *o, NCD_string_id_t func_name_id)
{
    DebugObject_Access(&o->d_obj);
    
    NCDModuleIndex__FuncHashRef lookup_ref = NCDModuleIndex__FuncHash_Lookup(&o->func_hash, o, func_name_id);
    if (lookup_ref.link < 0) {
        return NULL;
    }
    
    return &lookup_ref.ptr->ifunc;
}
