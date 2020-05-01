/**
 * @file CHash_header.h
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

// Preprocessor inputs:
// CHASH_PARAM_NAME - name of this data structure
// CHASH_PARAM_ENTRY - type of entry
// CHASH_PARAM_LINK - type of entry link (usually a pointer or index to an array)
// CHASH_PARAM_KEY - type of key
// CHASH_PARAM_ARG - type of argument pass through to comparisons
// CHASH_PARAM_NULL - invalid link
// CHASH_PARAM_DEREF(arg, link) - dereference a non-null link
// CHASH_PARAM_ENTRYHASH(arg, entry) - hash function for entries; returns size_t
// CHASH_PARAM_KEYHASH(arg, key) - hash function for keys; returns size_t
// CHASH_PARAM_ENTRYHASH_IS_CHEAP - define to 1 if CHASH_PARAM_ENTRYHASH is cheap (e.g. hashes are precomputed)
// CHASH_PARAM_COMPARE_ENTRIES(arg, entry1, entry2) - compares two entries; returns 1 for equality, 0 otherwise
// CHASH_PARAM_COMPARE_KEY_ENTRY(arg, key1, entry2) - compares key and entry; returns 1 for equality, 0 otherwise
// CHASH_PARAM_ENTRY_NEXT - next member in entry

#ifndef BADVPN_CHASH_H
#error CHash.h has not been included
#endif

// types
#define CHash CHASH_PARAM_NAME
#define CHashEntry CHASH_PARAM_ENTRY
#define CHashLink CHASH_PARAM_LINK
#define CHashRef MERGE(CHash, Ref)
#define CHashArg CHASH_PARAM_ARG
#define CHashKey CHASH_PARAM_KEY

// non-object public functions
#define CHashNullLink MERGE(CHash, NullLink)
#define CHashNullRef MERGE(CHash, NullRef)
#define CHashIsNullLink MERGE(CHash, IsNullLink)
#define CHashIsNullRef MERGE(CHash, IsNullRef)
#define CHashDerefMayNull MERGE(CHash, DerefMayNull)
#define CHashDerefNonNull MERGE(CHash, DerefNonNull)

// public functions
#define CHash_Init MERGE(CHash, _Init)
#define CHash_Free MERGE(CHash, _Free)
#define CHash_Insert MERGE(CHash, _Insert)
#define CHash_InsertMulti MERGE(CHash, _InsertMulti)
#define CHash_Remove MERGE(CHash, _Remove)
#define CHash_Lookup MERGE(CHash, _Lookup)
#define CHash_GetNextEqual MERGE(CHash, _GetNextEqual)
#define CHash_MultiplyBuckets MERGE(CHash, _MultiplyBuckets)
#define CHash_Verify MERGE(CHash, _Verify)

// private things
#define CHash_next(entry) ((entry).ptr->CHASH_PARAM_ENTRY_NEXT)
#define CHash_assert_valid_entry MERGE(CHash, _assert_valid_entry)
