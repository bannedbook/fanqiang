/**
 * @file SAvl_impl.h
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

#include "SAvl_tree.h"
#include <structure/CAvl_impl.h>

static void SAvl_Init (SAvl *o)
{
    SAvl__Tree_Init(&o->tree);
}

static int SAvl_Insert (SAvl *o, SAvlArg arg, SAvlEntry *entry, SAvlEntry **out_existing)
{
    ASSERT(entry)
#if SAVL_PARAM_FEATURE_COUNTS
    ASSERT(SAvl_Count(o, arg) < SAVL_PARAM_VALUE_COUNT_MAX)
#endif
    
    SAvl__TreeRef out_ref;
    int res = SAvl__Tree_Insert(&o->tree, arg, SAvl__TreeDeref(arg, entry), &out_ref);
    
    if (out_existing) {
        *out_existing = out_ref.link;
    }
    
    return res;
}

static void SAvl_Remove (SAvl *o, SAvlArg arg, SAvlEntry *entry)
{
    ASSERT(entry)
    
    SAvl__Tree_Remove(&o->tree, arg, SAvl__TreeDeref(arg, entry));
}

#if !SAVL_PARAM_FEATURE_NOKEYS

static SAvlEntry * SAvl_Lookup (const SAvl *o, SAvlArg arg, SAvlKey key)
{
    SAvl__TreeRef ref = SAvl__Tree_Lookup(&o->tree, arg, key);
    return ref.link;
}

static SAvlEntry * SAvl_LookupExact (const SAvl *o, SAvlArg arg, SAvlKey key)
{
    SAvl__TreeRef ref = SAvl__Tree_LookupExact(&o->tree, arg, key);
    return ref.link;
}

static SAvlEntry * SAvl_GetFirstGreater (const SAvl *o, SAvlArg arg, SAvlKey key)
{
    SAvl__TreeRef ref = SAvl__Tree_GetFirstGreater(&o->tree, arg, key);
    return ref.link;
}

static SAvlEntry * SAvl_GetLastLesser (const SAvl *o, SAvlArg arg, SAvlKey key)
{
    SAvl__TreeRef ref = SAvl__Tree_GetLastLesser(&o->tree, arg, key);
    return ref.link;
}

static SAvlEntry * SAvl_GetFirstGreaterEqual (const SAvl *o, SAvlArg arg, SAvlKey key)
{
    SAvl__TreeRef ref = SAvl__Tree_GetFirstGreaterEqual(&o->tree, arg, key);
    return ref.link;
}

static SAvlEntry * SAvl_GetLastLesserEqual (const SAvl *o, SAvlArg arg, SAvlKey key)
{
    SAvl__TreeRef ref = SAvl__Tree_GetLastLesserEqual(&o->tree, arg, key);
    return ref.link;
}

#endif

static SAvlEntry * SAvl_GetFirst (const SAvl *o, SAvlArg arg)
{
    SAvl__TreeRef ref = SAvl__Tree_GetFirst(&o->tree, arg);
    return ref.link;
}

static SAvlEntry * SAvl_GetLast (const SAvl *o, SAvlArg arg)
{
    SAvl__TreeRef ref = SAvl__Tree_GetLast(&o->tree, arg);
    return ref.link;
}

static SAvlEntry * SAvl_GetNext (const SAvl *o, SAvlArg arg, SAvlEntry *entry)
{
    ASSERT(entry)
    
    SAvl__TreeRef ref = SAvl__Tree_GetNext(&o->tree, arg, SAvl__TreeDeref(arg, entry));
    return ref.link;
}

static SAvlEntry * SAvl_GetPrev (const SAvl *o, SAvlArg arg, SAvlEntry *entry)
{
    ASSERT(entry)
    
    SAvl__TreeRef ref = SAvl__Tree_GetPrev(&o->tree, arg, SAvl__TreeDeref(arg, entry));
    return ref.link;
}

static int SAvl_IsEmpty (const SAvl *o)
{
    return SAvl__Tree_IsEmpty(&o->tree);
}

static void SAvl_Verify (const SAvl *o, SAvlArg arg)
{
    return SAvl__Tree_Verify(&o->tree, arg);
}

#if SAVL_PARAM_FEATURE_COUNTS

static SAvlCount SAvl_Count (const SAvl *o, SAvlArg arg)
{
    return SAvl__Tree_Count(&o->tree, arg);
}

static SAvlCount SAvl_IndexOf (const SAvl *o, SAvlArg arg, SAvlEntry *entry)
{
    ASSERT(entry)
    
    return SAvl__Tree_IndexOf(&o->tree, arg, SAvl__TreeDeref(arg, entry));
}

static SAvlEntry * SAvl_GetAt (const SAvl *o, SAvlArg arg, SAvlCount index)
{
    SAvl__TreeRef ref = SAvl__Tree_GetAt(&o->tree, arg, index);
    return ref.link;
}

#endif

#include "SAvl_footer.h"
