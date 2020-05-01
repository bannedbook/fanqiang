/**
 * @file NCDInterpProcess.h
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

#ifndef BADVPN_NCDINTERPPROCESS_H
#define BADVPN_NCDINTERPPROCESS_H

#include <stddef.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <ncd/NCDAst.h>
#include <ncd/NCDVal.h>
#include <ncd/NCDEvaluator.h>
#include <ncd/NCDModule.h>
#include <ncd/NCDModuleIndex.h>
#include <ncd/NCDStringIndex.h>

struct NCDInterpProcess__stmt;

/**
 * A data structure which contains information about a process or
 * template, suitable for efficient interpretation. These structures
 * are built at startup from the program AST for all processes and
 * templates by \link NCDInterpProg. They are not modified after
 * the program is loaded Inn case of template processes, the same
 * NCDInterpProcess is shared by all processes created from the same
 * template.
 */
typedef struct {
    struct NCDInterpProcess__stmt *stmts;
    char *name;
    int num_stmts;
    int prealloc_size;
    int is_template;
    int *hash_buckets;
    size_t num_hash_buckets;
    void *cache;
    DebugObject d_obj;
} NCDInterpProcess;

int NCDInterpProcess_Init (NCDInterpProcess *o, NCDProcess *process, NCDStringIndex *string_index, NCDEvaluator *eval, NCDModuleIndex *module_index) WARN_UNUSED;
void NCDInterpProcess_Free (NCDInterpProcess *o);
int NCDInterpProcess_FindStatement (NCDInterpProcess *o, int from_index, NCD_string_id_t name);
const char * NCDInterpProcess_StatementCmdName (NCDInterpProcess *o, int i, NCDStringIndex *string_index);
void NCDInterpProcess_StatementObjNames (NCDInterpProcess *o, int i, const NCD_string_id_t **out_objnames, size_t *out_num_objnames);
const struct NCDInterpModule * NCDInterpProcess_StatementGetSimpleModule (NCDInterpProcess *o, int i, NCDStringIndex *string_index, NCDModuleIndex *module_index);
const struct NCDInterpModule * NCDInterpProcess_StatementGetMethodModule (NCDInterpProcess *o, int i, NCD_string_id_t obj_type, NCDModuleIndex *module_index);
NCDEvaluatorExpr * NCDInterpProcess_GetStatementArgsExpr (NCDInterpProcess *o, int i);
void NCDInterpProcess_StatementBumpAllocSize (NCDInterpProcess *o, int i, int alloc_size);
int NCDInterpProcess_PreallocSize (NCDInterpProcess *o);
int NCDInterpProcess_StatementPreallocSize (NCDInterpProcess *o, int i);
int NCDInterpProcess_StatementPreallocOffset (NCDInterpProcess *o, int i);
const char * NCDInterpProcess_Name (NCDInterpProcess *o);
int NCDInterpProcess_IsTemplate (NCDInterpProcess *o);
int NCDInterpProcess_NumStatements (NCDInterpProcess *o);
int NCDInterpProcess_CachePush (NCDInterpProcess *o, void *elem) WARN_UNUSED;
void * NCDInterpProcess_CachePull (NCDInterpProcess *o);

#endif
