/**
 * @file NCDModuleIndex.h
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

#ifndef BADVPN_NCDMODULEINDEX_H
#define BADVPN_NCDMODULEINDEX_H

#include <misc/debug.h>
#include <structure/BAVL.h>
#include <structure/CHash.h>
#include <structure/LinkedList0.h>
#include <structure/Vector.h>
#include <base/DebugObject.h>
#include <ncd/NCDStringIndex.h>
#include <ncd/NCDModule.h>
#include <ncd/NCDMethodIndex.h>

#define NCDMODULEINDEX_MODULES_HASH_SIZE 512
#define NCDMODULEINDEX_FUNCTIONS_VEC_INITIAL_SIZE 32
#define NCDMODULEINDEX_FUNCTIONS_HASH_SIZE 64

struct NCDModuleIndex_module {
    struct NCDInterpModule imodule;
    struct NCDModuleIndex_module *hash_next;
    int method_id;
};

#ifndef NDEBUG
struct NCDModuleIndex_base_type {
    const char *base_type;
    struct NCDModuleIndex_group *group;
    BAVLNode base_types_tree_node;
};
#endif

struct NCDModuleIndex_group {
    LinkedList0Node groups_list_node;
    struct NCDInterpModuleGroup igroup;
    struct NCDModuleIndex_module modules[];
};

struct NCDModuleIndex__Func {
    struct NCDInterpFunction ifunc;
    int hash_next;
};

typedef struct NCDModuleIndex_module *NCDModuleIndex__mhash_link;
typedef const char *NCDModuleIndex__mhash_key;

typedef struct NCDModuleIndex_s NCDModuleIndex;

#include "NCDModuleIndex_mhash.h"
#include <structure/CHash_decl.h>

#include "NCDModuleIndex_func_vec.h"
#include <structure/Vector_decl.h>

#include "NCDModuleIndex_fhash.h"
#include <structure/CHash_decl.h>

struct NCDModuleIndex_s {
    NCDModuleIndex__MHash modules_hash;
#ifndef NDEBUG
    BAVL base_types_tree;
#endif
    LinkedList0 groups_list;
    NCDMethodIndex method_index;
    NCDModuleIndex__FuncVec func_vec;
    NCDModuleIndex__FuncHash func_hash;
    DebugObject d_obj;
};

int NCDModuleIndex_Init (NCDModuleIndex *o, NCDStringIndex *string_index) WARN_UNUSED;
void NCDModuleIndex_Free (NCDModuleIndex *o);
int NCDModuleIndex_AddGroup (NCDModuleIndex *o, const struct NCDModuleGroup *group, const struct NCDModuleInst_iparams *iparams, NCDStringIndex *string_index) WARN_UNUSED;
const struct NCDInterpModule * NCDModuleIndex_FindModule (NCDModuleIndex *o, const char *type);
int NCDModuleIndex_GetMethodNameId (NCDModuleIndex *o, const char *method_name);
const struct NCDInterpModule * NCDModuleIndex_GetMethodModule (NCDModuleIndex *o, NCD_string_id_t obj_type, int method_name_id);
const struct NCDInterpFunction * NCDModuleIndex_FindFunction (NCDModuleIndex *o, NCD_string_id_t func_name_id);

#endif
