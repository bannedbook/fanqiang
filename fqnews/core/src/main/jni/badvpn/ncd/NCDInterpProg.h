/**
 * @file NCDInterpProg.h
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

#ifndef BADVPN_NCDINTERPPROG_H
#define BADVPN_NCDINTERPPROG_H

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <ncd/NCDAst.h>
#include <ncd/NCDInterpProcess.h>
#include <ncd/NCDStringIndex.h>
#include <structure/CHash.h>

struct NCDInterpProg__process {
    NCD_string_id_t name;
    NCDInterpProcess iprocess;
    int hash_next;
};

typedef struct NCDInterpProg__process NCDInterpProg__hashentry;
typedef struct NCDInterpProg__process *NCDInterpProg__hasharg;

#include "NCDInterpProg_hash.h"
#include <structure/CHash_decl.h>

typedef struct {
    struct NCDInterpProg__process *procs;
    int num_procs;
    NCDInterpProg__Hash hash;
    DebugObject d_obj;
} NCDInterpProg;

int NCDInterpProg_Init (NCDInterpProg *o, NCDProgram *prog, NCDStringIndex *string_index, NCDEvaluator *eval, NCDModuleIndex *module_index) WARN_UNUSED;
void NCDInterpProg_Free (NCDInterpProg *o);
NCDInterpProcess * NCDInterpProg_FindProcess (NCDInterpProg *o, NCD_string_id_t name);

#endif
