/**
 * @file SAvl_decl.h
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

#include "SAvl_header.h"

typedef SAvlEntry *SAvl__TreeLink;

#include "SAvl_tree.h"
#include <structure/CAvl_decl.h>

typedef struct {
    SAvl__Tree tree;
} SAvl;

typedef struct {
    SAvlEntry *child[2];
    SAvlEntry *parent;
    int8_t balance;
#if SAVL_PARAM_FEATURE_COUNTS
    SAvlCount count;
#endif
} SAvlNode;

static void SAvl_Init (SAvl *o);
static int SAvl_Insert (SAvl *o, SAvlArg arg, SAvlEntry *entry, SAvlEntry **out_existing);
static void SAvl_Remove (SAvl *o, SAvlArg arg, SAvlEntry *entry);
#if !SAVL_PARAM_FEATURE_NOKEYS
static SAvlEntry * SAvl_Lookup (const SAvl *o, SAvlArg arg, SAvlKey key);
static SAvlEntry * SAvl_LookupExact (const SAvl *o, SAvlArg arg, SAvlKey key);
static SAvlEntry * SAvl_GetFirstGreater (const SAvl *o, SAvlArg arg, SAvlKey key);
static SAvlEntry * SAvl_GetLastLesser (const SAvl *o, SAvlArg arg, SAvlKey key);
static SAvlEntry * SAvl_GetFirstGreaterEqual (const SAvl *o, SAvlArg arg, SAvlKey key);
static SAvlEntry * SAvl_GetLastLesserEqual (const SAvl *o, SAvlArg arg, SAvlKey key);
#endif
static SAvlEntry * SAvl_GetFirst (const SAvl *o, SAvlArg arg);
static SAvlEntry * SAvl_GetLast (const SAvl *o, SAvlArg arg);
static SAvlEntry * SAvl_GetNext (const SAvl *o, SAvlArg arg, SAvlEntry *entry);
static SAvlEntry * SAvl_GetPrev (const SAvl *o, SAvlArg arg, SAvlEntry *entry);
static int SAvl_IsEmpty (const SAvl *o);
static void SAvl_Verify (const SAvl *o, SAvlArg arg);
#if SAVL_PARAM_FEATURE_COUNTS
static SAvlCount SAvl_Count (const SAvl *o, SAvlArg arg);
static SAvlCount SAvl_IndexOf (const SAvl *o, SAvlArg arg, SAvlEntry *entry);
static SAvlEntry * SAvl_GetAt (const SAvl *o, SAvlArg arg, SAvlCount index);
#endif

#include "SAvl_footer.h"
