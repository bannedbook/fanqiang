/**
 * @file NCDValCons.h
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

#ifndef BADVPN_NCDVALCONS_H
#define BADVPN_NCDVALCONS_H

#include <limits.h>
#include <stdint.h>

#include <misc/debug.h>
#include <ncd/NCDVal.h>

struct NCDValCons__temp_elem {
    NCDValSafeRef ref;
    int next;
};

/**
 * Value constructor; implements a mechanism for efficiently constructing
 * NCD values into {@link NCDVal} compact representation, but without
 * having to know the number of list or map elements in advance.
 * For the purpose of value construction, values are representing using
 * {@link NCDValConsVal} objects.
 */
typedef struct {
    NCDValMem *mem;
    struct NCDValCons__temp_elem *elems;
    int elems_size;
    int elems_capacity;
} NCDValCons;

#define NCDVALCONS_TYPE_COMPLETE 1
#define NCDVALCONS_TYPE_INCOMPLETE_LIST 2
#define NCDVALCONS_TYPE_INCOMPLETE_MAP 3

/**
 * Abstract handle which represents a value during constuction via
 * {@link NCDValCons}. 
 */
typedef struct {
    int cons_type;
    union {
        NCDValSafeRef complete_ref;
        struct {
            int elems_idx;
            int count;
        } incomplete;
    } u;
} NCDValConsVal;

#define NCDVALCONS_ERROR_MEMORY 1
#define NCDVALCONS_ERROR_DUPLICATE_KEY 2
#define NCDVALCONS_ERROR_DEPTH 3

/**
 * Initializes a value constructor.
 * 
 * @param o value constructor to initialize
 * @param mem memory object where values will be stored into
 * @return 1 on success, 0 on failure
 */
int NCDValCons_Init (NCDValCons *o, NCDValMem *mem) WARN_UNUSED;

/**
 * Frees the value constructor. This only means the constuctor does
 * not exist any more; any values constructed and completed using
 * {@link NCDValCons_Complete} remain in the memory object.
 * 
 * @param o value constructor to free
 */
void NCDValCons_Free (NCDValCons *o);

/**
 * Creates a new string value with the given data.
 * 
 * @param o value constructor
 * @param data pointer to string data. This must not point into the
 *             memory object the value constructor is using. The data
 *             is copied.
 * @param len length of the string
 * @param out on success, *out will be set to a handle representing
 *            the new string
 * @param out_error on failure, *out_error will be set to an error code
 * @return 1 on success, 0 on failure
 */
int NCDValCons_NewString (NCDValCons *o, const uint8_t *data, size_t len, NCDValConsVal *out, int *out_error) WARN_UNUSED;

/**
 * Creates an empty list value.
 * 
 * @param o value constructor
 * @param out *out will be set to a handle representing the new list
 */
void NCDValCons_NewList (NCDValCons *o, NCDValConsVal *out);

/**
 * Creates an empty map value.
 * 
 * @param o value constructor
 * @param out *out will be set to a handle representing the new map
 */
void NCDValCons_NewMap (NCDValCons *o, NCDValConsVal *out);

/**
 * Prepends an element to a list value.
 * 
 * @param o value constructor
 * @param list pointer to the handle representing the list. On success,
 *             the handle will be modified, and the old handle must not
 *             be used any more.
 * @param elem handle representing the value to be prepended. This handle
 *             must not be used any more after being prepended to the list.
 * @param out_error on failure, *out_error will be set to an error code
 * @return 1 on success, 0 on failure
 */
int NCDValCons_ListPrepend (NCDValCons *o, NCDValConsVal *list, NCDValConsVal elem, int *out_error) WARN_UNUSED;

/**
 * Inserts an entry into a map value.
 * 
 * @param o value constructor
 * @param map pointer to the handle representing the map. On success,
 *             the handle will be modified, and the old handle must not
 *             be used any more.
 * @param key handle representing the key of the entry. This handle
 *            must not be used any more after being inserted into the map.
 * @param value handle representing the value of the entry. This handle
 *              must not be used any more after being inserted into the
 *              map.
 * @param out_error on failure, *out_error will be set to an error code
 * @return 1 on success, 0 on failure
 */
int NCDValCons_MapInsert (NCDValCons *o, NCDValConsVal *map, NCDValConsVal key, NCDValConsVal value, int *out_error) WARN_UNUSED;

/**
 * Completes a value represented by a {@link NCDValConsVal} handle,
 * producing a {@link NCDValRef} object which refers to this value within
 * the memory object.
 * 
 * @param o value constructor
 * @param val handle representing the value to be completed. After a value
 *            is completed, the handle must not be used any more.
 * @param out on success, *out will be set to a {@link NCDValRef} object
 *            referencing the completed value
 * @param out_error on failure, *out_error will be set to an error code
 * @return 1 on success, 0 on failure
 */
int NCDValCons_Complete (NCDValCons *o, NCDValConsVal val, NCDValRef *out, int *out_error) WARN_UNUSED;

#endif
