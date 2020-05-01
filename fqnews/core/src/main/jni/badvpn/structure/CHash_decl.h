/**
 * @file CHash_decl.h
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

#include "CHash_header.h"

typedef struct {
    CHashLink *buckets;
    size_t num_buckets;
} CHash;

typedef struct {
    CHashEntry *ptr;
    CHashLink link;
} CHashRef;

static CHashLink CHashNullLink (void);
static CHashRef CHashNullRef (void);
static int CHashIsNullLink (CHashLink link);
static int CHashIsNullRef (CHashRef ref);
static CHashRef CHashDerefMayNull (CHashArg arg, CHashLink link);
static CHashRef CHashDerefNonNull (CHashArg arg, CHashLink link);

static int CHash_Init (CHash *o, size_t num_buckets);
static void CHash_Free (CHash *o);
static int CHash_Insert (CHash *o, CHashArg arg, CHashRef entry, CHashRef *out_existing);
static void CHash_InsertMulti (CHash *o, CHashArg arg, CHashRef entry);
static void CHash_Remove (CHash *o, CHashArg arg, CHashRef entry);
static CHashRef CHash_Lookup (const CHash *o, CHashArg arg, CHashKey key);
static CHashRef CHash_GetNextEqual (const CHash *o, CHashArg arg, CHashRef entry);
static int CHash_MultiplyBuckets (CHash *o, CHashArg arg, int exp);
static void CHash_Verify (const CHash *o, CHashArg arg);

#include "CHash_footer.h"
