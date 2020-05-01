/**
 * @file CHash_impl.h
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

static void CHash_assert_valid_entry (CHashArg arg, CHashRef entry)
{
    ASSERT(entry.link != CHashNullLink())
    ASSERT(entry.ptr == CHASH_PARAM_DEREF(arg, entry.link))
}

static CHashLink CHashNullLink (void)
{
    return CHASH_PARAM_NULL;
}

static CHashRef CHashNullRef (void)
{
    CHashRef entry = {NULL, CHashNullLink()};
    return entry;
}

static int CHashIsNullLink (CHashLink link)
{
    return (link == CHashNullLink());
}

static int CHashIsNullRef (CHashRef ref)
{
    return CHashIsNullLink(ref.link);
}

static CHashRef CHashDerefMayNull (CHashArg arg, CHashLink link)
{
    if (link == CHashNullLink()) {
        return CHashNullRef();
    }
    
    CHashRef entry = {CHASH_PARAM_DEREF(arg, link), link};
    ASSERT(entry.ptr)
    
    return entry;
}

static CHashRef CHashDerefNonNull (CHashArg arg, CHashLink link)
{
    ASSERT(link != CHashNullLink())
    
    CHashRef entry = {CHASH_PARAM_DEREF(arg, link), link};
    ASSERT(entry.ptr)
    
    return entry;
}

static int CHash_Init (CHash *o, size_t num_buckets)
{
    if (num_buckets == 0) {
        num_buckets = 1;
    }
    
    o->num_buckets = num_buckets;
    
    o->buckets = (CHashLink *)BAllocArray(o->num_buckets, sizeof(o->buckets[0])); 
    if (!o->buckets) {
        return 0;
    }
    
    for (size_t i = 0; i < o->num_buckets; i++) {
        o->buckets[i] = CHashNullLink();
    }
    
    return 1;
}

static void CHash_Free (CHash *o)
{
    BFree(o->buckets);
}

static int CHash_Insert (CHash *o, CHashArg arg, CHashRef entry, CHashRef *out_existing)
{
    CHash_assert_valid_entry(arg, entry);
    
    size_t index = CHASH_PARAM_ENTRYHASH(arg, entry) % o->num_buckets;
    
    CHashLink link = o->buckets[index];
    while (link != CHashNullLink()) {
        CHashRef cur = CHashDerefNonNull(arg, link);
        if (CHASH_PARAM_COMPARE_ENTRIES(arg, cur, entry)) {
            if (out_existing) {
                *out_existing = cur;
            }
            return 0;
        }
        link = CHash_next(cur);
    }
    
    CHash_next(entry) = o->buckets[index];
    o->buckets[index] = entry.link;
    
    return 1;
}

static void CHash_InsertMulti (CHash *o, CHashArg arg, CHashRef entry)
{
    CHash_assert_valid_entry(arg, entry);
    
    size_t index = CHASH_PARAM_ENTRYHASH(arg, entry) % o->num_buckets;
    
    CHashRef prev = CHashNullRef();
    CHashLink link = o->buckets[index];
    while (link != CHashNullLink()) {
        CHashRef cur = CHashDerefNonNull(arg, link);
        if (CHASH_PARAM_COMPARE_ENTRIES(arg, cur, entry)) {
            break;
        }
        prev = cur;
        link = CHash_next(cur);
    }
    
    if (link == CHashNullLink() || prev.link == CHashNullLink()) {
        CHash_next(entry) = o->buckets[index];
        o->buckets[index] = entry.link;
    } else {
        CHash_next(entry) = link;
        CHash_next(prev) = entry.link;
    }
}

static void CHash_Remove (CHash *o, CHashArg arg, CHashRef entry)
{
    CHash_assert_valid_entry(arg, entry);
    
    size_t index = CHASH_PARAM_ENTRYHASH(arg, entry) % o->num_buckets;
    
    CHashRef prev = CHashNullRef();
    CHashLink link = o->buckets[index];
    while (link != entry.link) {
        CHashRef cur = CHashDerefNonNull(arg, link);
        prev = cur;
        link = CHash_next(cur);
        ASSERT(link != CHashNullLink())
    }
    
    if (prev.link == CHashNullLink()) {
        o->buckets[index] = CHash_next(entry);
    } else {
        CHash_next(prev) = CHash_next(entry);
    }
}

static CHashRef CHash_Lookup (const CHash *o, CHashArg arg, CHashKey key) 
{
    size_t hash = CHASH_PARAM_KEYHASH(arg, key);
    size_t index = hash % o->num_buckets;
    
    CHashLink link = o->buckets[index];
    while (link != CHashNullLink()) {
        CHashRef cur = CHashDerefNonNull(arg, link);
#if CHASH_PARAM_ENTRYHASH_IS_CHEAP
        if (CHASH_PARAM_ENTRYHASH(arg, cur) == hash && CHASH_PARAM_COMPARE_KEY_ENTRY(arg, key, cur)) {
#else
        if (CHASH_PARAM_COMPARE_KEY_ENTRY(arg, key, cur)) {
#endif
            return cur;
        }
        link = CHash_next(cur);
    }
    
    return CHashNullRef();
}

static CHashRef CHash_GetNextEqual (const CHash *o, CHashArg arg, CHashRef entry)
{
    CHash_assert_valid_entry(arg, entry);
    
    CHashLink next = CHash_next(entry);
    
    if (next == CHashNullLink()) {
        return CHashNullRef();
    }
    
    CHashRef next_ref = CHashDerefNonNull(arg, next);
    if (!CHASH_PARAM_COMPARE_ENTRIES(arg, next_ref, entry)) {
        return CHashNullRef();
    }
    
    return next_ref;
}

static int CHash_MultiplyBuckets (CHash *o, CHashArg arg, int exp)
{
    ASSERT(exp > 0)
    
    size_t new_num_buckets = o->num_buckets;
    while (exp-- > 0) {
        if (new_num_buckets > SIZE_MAX / 2) {
            return 0;
        }
        new_num_buckets *= 2;
    }
    
    CHashLink *new_buckets = (CHashLink *)BReallocArray(o->buckets, new_num_buckets, sizeof(new_buckets[0]));
    if (!new_buckets) {
        return 0;
    }
    o->buckets = new_buckets;
    
    for (size_t i = o->num_buckets; i < new_num_buckets; i++) {
        o->buckets[i] = CHashNullLink();
    }
    
    for (size_t i = 0; i < o->num_buckets; i++) {
        CHashRef prev = CHashNullRef();
        CHashLink link = o->buckets[i];
        
        while (link != CHashNullLink()) {
            CHashRef cur = CHashDerefNonNull(arg, link);
            link = CHash_next(cur);
            
            size_t new_index = CHASH_PARAM_ENTRYHASH(arg, cur) % new_num_buckets;
            if (new_index == i) {
                prev = cur;
                continue;
            }
            
            if (CHashIsNullRef(prev)) {
                o->buckets[i] = CHash_next(cur);
            } else {
                CHash_next(prev) = CHash_next(cur);
            }
            
            CHash_next(cur) = o->buckets[new_index];
            o->buckets[new_index] = cur.link;
        }
    }
    
    for (size_t i = o->num_buckets; i < new_num_buckets; i++) {
        CHashLink new_bucket_link = CHashNullLink();
        
        CHashLink link = o->buckets[i];
        while (link != CHashNullLink()) {
            CHashRef cur = CHashDerefNonNull(arg, link);
            link = CHash_next(cur);
            
            CHash_next(cur) = new_bucket_link;
            new_bucket_link = cur.link;
        }
        
        o->buckets[i] = new_bucket_link;
    }
    
    o->num_buckets = new_num_buckets;
    
    return 1;
}

static void CHash_Verify (const CHash *o, CHashArg arg)
{
    ASSERT_FORCE(o->num_buckets > 0)
    ASSERT_FORCE(o->buckets)
    
    for (size_t i = 0; i < o->num_buckets; i++) {
        CHashRef cur = CHashDerefMayNull(arg, o->buckets[i]);
        CHashRef same_first = cur;
        
        while (!CHashIsNullRef(cur)) {
            size_t index = CHASH_PARAM_ENTRYHASH(arg, cur) % o->num_buckets;
            ASSERT_FORCE(index == i)
            
            if (!CHASH_PARAM_COMPARE_ENTRIES(arg, cur, same_first)) {
                same_first = cur;
            }
            
            CHashRef ccur = CHashDerefNonNull(arg, o->buckets[i]);
            while (ccur.link != same_first.link) {
                ASSERT_FORCE(!CHASH_PARAM_COMPARE_ENTRIES(arg, ccur, cur))
                ccur = CHashDerefMayNull(arg, CHash_next(ccur));
            }
            
            cur = CHashDerefMayNull(arg, CHash_next(cur));
        }
    }
}

#include "CHash_footer.h"
