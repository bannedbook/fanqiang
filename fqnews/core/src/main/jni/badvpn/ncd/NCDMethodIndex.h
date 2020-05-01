/**
 * @file NCDMethodIndex.h
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

#ifndef BADVPN_NCDMETHODINDEX_H
#define BADVPN_NCDMETHODINDEX_H

#include <misc/debug.h>
#include <structure/CHash.h>
#include <ncd/NCDModule.h>
#include <ncd/NCDStringIndex.h>

#define NCDMETHODINDEX_NUM_EXPECTED_METHOD_NAMES 64
#define NCDMETHODINDEX_NUM_EXPECTED_ENTRIES 64

struct NCDMethodIndex__method_name {
    char *method_name;
    int first_entry;
    int hash_next;
};

struct NCDMethodIndex__entry {
    NCD_string_id_t obj_type;
    const struct NCDInterpModule *module;
    int next;
};

typedef struct NCDMethodIndex__method_name NCDMethodIndex__hashentry;
typedef const char *NCDMethodIndex__hashkey;
typedef struct NCDMethodIndex__method_name *NCDMethodIndex__hasharg;

#include "NCDMethodIndex_hash.h"
#include <structure/CHash_decl.h>

/**
 * The method index associates (object_type, method_name) pairs to pointers
 * to corresponding \link NCDInterpModule structures (whose type strings would
 * be "object_type::method_name").
 * More precisely, the method names are represented as indices into an
 * internal array, which allows very efficient lookup when the method names
 * are known in advance, but not the object types.
 */
typedef struct {
    struct NCDMethodIndex__method_name *names;
    struct NCDMethodIndex__entry *entries;
    int names_capacity;
    int entries_capacity;
    int num_names;
    int num_entries;
    NCDMethodIndex__Hash hash;
    NCDStringIndex *string_index;
} NCDMethodIndex;

/**
 * Initializes the method index.
 * 
 * @return 1 on success, 0 on failure
 */
int NCDMethodIndex_Init (NCDMethodIndex *o, NCDStringIndex *string_index) WARN_UNUSED;

/**
 * Frees the method index.
 */
void NCDMethodIndex_Free (NCDMethodIndex *o);

/**
 * Adds a method to the index.
 * Duplicate methods will not be detected here.
 * 
 * @param obj_type object type of method, e.g. "cat" in "cat::meow".
 *                 Must not be NULL. Does not have to be null-terminated.
 * @param obj_type_len number of characters in obj_type
 * @param method_name name of method, e.g. "meow" in "cat::meow".
 *                    Must not be NULL.
 * @param module pointer to module structure. Must not be NULL.
 * @return on success, a non-negative identifier; on failure, -1
 */
int NCDMethodIndex_AddMethod (NCDMethodIndex *o, const char *obj_type, size_t obj_type_len, const char *method_name, const struct NCDInterpModule *module);

/**
 * Removes a method from the index.
 * 
 * @param method_name_id method name identifier
 */
void NCDMethodIndex_RemoveMethod (NCDMethodIndex *o, int method_name_id);

/**
 * Obtains an internal integer identifier for a method name. The intention is that
 * this is stored and later passed to \link NCDMethodIndex_GetMethodModule for
 * efficient lookup of modules corresponding to methods.
 * 
 * @param method_name name of method, e.g. "meow" in "cat::meow".
 *                    Must not be NULL.
 * @return non-negative integer on success, -1 on failure
 */
int NCDMethodIndex_GetMethodNameId (NCDMethodIndex *o, const char *method_name);

/**
 * Looks up the module corresponding to a method. The method name is passed as an
 * identifier obtained from \link NCDMethodIndex_GetMethodNameId.
 * 
 * @param obj_type object type of method, e.g. "cat" in "cat::meow", as a string
 *                 identifier via {@link NCDStringIndex}
 * @param method_name_id method name identifier. Must have been previously returned
 *                       by a successfull call of \link NCDMethodIndex_GetMethodNameId.
 * @return module pointer, or NULL if no such method exists
 */
const struct NCDInterpModule * NCDMethodIndex_GetMethodModule (NCDMethodIndex *o, NCD_string_id_t obj_type, int method_name_id);

#endif
