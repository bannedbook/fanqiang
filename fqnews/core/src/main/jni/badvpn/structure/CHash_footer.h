/**
 * @file CHash_footer.h
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

// preprocessor inputs
#undef CHASH_PARAM_NAME
#undef CHASH_PARAM_ENTRY
#undef CHASH_PARAM_LINK
#undef CHASH_PARAM_KEY
#undef CHASH_PARAM_ARG
#undef CHASH_PARAM_NULL
#undef CHASH_PARAM_DEREF
#undef CHASH_PARAM_ENTRYHASH
#undef CHASH_PARAM_KEYHASH
#undef CHASH_PARAM_ENTRYHASH_IS_CHEAP
#undef CHASH_PARAM_COMPARE_ENTRIES
#undef CHASH_PARAM_COMPARE_KEY_ENTRY
#undef CHASH_PARAM_ENTRY_NEXT

// types
#undef CHash
#undef CHashEntry
#undef CHashLink
#undef CHashRef
#undef CHashArg
#undef CHashKey

// non-object public functions
#undef CHashNullLink
#undef CHashNullRef
#undef CHashIsNullLink
#undef CHashIsNullRef
#undef CHashDerefMayNull
#undef CHashDerefNonNull

// public functions
#undef CHash_Init
#undef CHash_Free
#undef CHash_Insert
#undef CHash_InsertMulti
#undef CHash_Remove
#undef CHash_Lookup
#undef CHash_GetNextEqual
#undef CHash_MultiplyBuckets
#undef CHash_Verify

// private things
#undef CHash_next
#undef CHash_assert_valid_entry
