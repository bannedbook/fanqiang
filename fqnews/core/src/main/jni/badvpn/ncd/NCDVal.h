/**
 * @file NCDVal.h
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

#ifndef BADVPN_NCDVAL_H
#define BADVPN_NCDVAL_H

#include <stddef.h>
#include <stdint.h>

#include <misc/debug.h>
#include <misc/BRefTarget.h>
#include <misc/memref.h>
#include <ncd/NCDStringIndex.h>
#include <ncd/NCDVal_types.h>

#define NCDVAL_STRING 1
#define NCDVAL_LIST 2
#define NCDVAL_MAP 3
#define NCDVAL_PLACEHOLDER 4

/**
 * Initializes a value memory object.
 * A value memory object holds memory for value structures. Values within
 * the memory are referenced using {@link NCDValRef} objects, which point
 * to values within memory objects.
 * 
 * Values may be added to a memory object using functions such as
 * {@link NCDVal_NewString}, {@link NCDVal_NewList} and {@link NCDVal_NewMap},
 * and {@link NCDVal_NewCopy}, which return references to the new values within
 * the memory object.
 * 
 * It is not possible to remove values from the memory object, or modify existing
 * values other than adding elements to pre-allocated slots in lists and maps.
 * Once a value is added, it will consume memory as long as its memory object
 * exists. This is by design - this code is intended and optimized for constructing
 * and passing around values, not for operating on them in place. In fact, al
 * values within a memory object are stored in a single memory buffer, as an
 * embedded data structure with relativepointers. For example, map values use an
 * embedded AVL tree.
 */
void NCDValMem_Init (NCDValMem *o, NCDStringIndex *string_index);

/**
 * Frees a value memory object.
 * All values within the memory object cease to exist, and any {@link NCDValRef}
 * object pointing to them must no longer be used.
 */
void NCDValMem_Free (NCDValMem *o);

/**
 * Initializes the memory object to be a copy of an existing memory object.
 * Value references from the original may be used if they are first turned
 * to {@link NCDValSafeRef} using {@link NCDVal_ToSafe} and back to
 * {@link NCDValRef} using {@link NCDVal_FromSafe} with the new memory object
 * specified. Alternatively, {@link NCDVal_Moved} can be used.
 * Returns 1 on success and 0 on failure.
 */
int NCDValMem_InitCopy (NCDValMem *o, NCDValMem *other) WARN_UNUSED;

/**
 * Get the string index of a value memory object.
 */
NCDStringIndex * NCDValMem_StringIndex (NCDValMem *o);

/**
 * Does nothing.
 * The value reference object must either point to a valid value within a valid
 * memory object, or must be an invalid reference (most functions operating on
 * {@link NCDValRef} implicitly require that).
 */
void NCDVal_Assert (NCDValRef val);

/**
 * Determines if a value reference is invalid.
 */
int NCDVal_IsInvalid (NCDValRef val);

/**
 * Determines if a value is a placeholder value.
 * The value reference must not be an invalid reference.
 */
int NCDVal_IsPlaceholder (NCDValRef val);

/**
 * Returns the type of the value reference, which must not be an invalid reference.
 * Possible values are NCDVAL_STRING, NCDVAL_LIST, NCDVAL_MAP and NCDVAL_PLACEHOLDER.
 * The placeholder type is only used internally in the interpreter for argument
 * resolution, and is never seen by modules; see {@link NCDVal_NewPlaceholder}.
 */
int NCDVal_Type (NCDValRef val);

/**
 * Returns an invalid reference.
 * An invalid reference must not be passed to any function here, except:
 *   {@link NCDVal_Assert}, {@link NCDVal_IsInvalid}, {@link NCDVal_ToSafe},
 *   {@link NCDVal_FromSafe}, {@link NCDVal_Moved}.
 */
NCDValRef NCDVal_NewInvalid (void);

/**
 * Returns a new placeholder value reference. A placeholder value is a valid value
 * containing an integer placeholder identifier.
 * This always succeeds; however, the caller must ensure the identifier is in the
 * range [0, NCDVAL_TOPPLID).
 * 
 * The placeholder type is only used internally in the interpreter for argument
 * resolution, and is never seen by modules.
 */
NCDValRef NCDVal_NewPlaceholder (NCDValMem *mem, int plid);

/**
 * Returns the indentifier of a placeholder value.
 * The value reference must point to a placeholder value.
 */
int NCDVal_PlaceholderId (NCDValRef val);

/**
 * Copies a value into the specified memory object. The source
 * must not be an invalid reference, however it may reside in any memory
 * object (including 'mem').
 * Returns a reference to the copied value. On out of memory, returns
 * an invalid reference.
 */
NCDValRef NCDVal_NewCopy (NCDValMem *mem, NCDValRef val);

/**
 * Compares two values, both of which must not be invalid references.
 * Returns -1, 0 or 1.
 */
int NCDVal_Compare (NCDValRef val1, NCDValRef val2);

/**
 * Converts a value reference to a safe referece format, which remains valid
 * if the memory object is moved (safe references do not contain a pointer
 * to the memory object, unlike {@link NCDValRef} references).
 */
NCDValSafeRef NCDVal_ToSafe (NCDValRef val);

/**
 * Converts a safe value reference to a normal value reference.
 * This should be used to recover references from safe references
 * after the memory object is moved.
 */
NCDValRef NCDVal_FromSafe (NCDValMem *mem, NCDValSafeRef sval);

/**
 * Fixes a value reference after its memory object was moved.
 */
NCDValRef NCDVal_Moved (NCDValMem *mem, NCDValRef val);

/**
 * Determines whether a safe reference is a placeholder.
 */
int NCDVal_IsSafeRefPlaceholder (NCDValSafeRef sval);

/**
 * Gets the placeholder ID of a placeholder safe reference.
 */
int NCDVal_GetSafeRefPlaceholderId (NCDValSafeRef sval);

/**
 * Determines if the value implements the String interface.
 * The value reference must not be an invalid reference.
 */
int NCDVal_IsString (NCDValRef val);

/**
 * Determines if the value is a StoredString.
 * A StoredString implements the String interface.
 * The value reference must not be an invalid reference.
 */
int NCDVal_IsStoredString (NCDValRef val);

/**
 * Determines if the value is an IdString. See {@link NCDVal_NewIdString}
 * for details.
 * An IdString implements the String interface.
 * The value reference must not be an invalid reference.
 */
int NCDVal_IsIdString (NCDValRef val);

/**
 * Determines if a value is an ExternalString.
 * See {@link NCDVal_NewExternalString} for details.
 * An ExternalString implements the String interface.
 * The value reference must not be an invalid reference.
 */
int NCDVal_IsExternalString (NCDValRef val);

/**
 * Determines if a value is a String which contains no null bytes.
 * The value reference must not be an invalid reference.
 */
int NCDVal_IsStringNoNulls (NCDValRef val);

/**
 * Equivalent to NCDVal_NewStringBin(mem, data, strlen(data)).
 */
NCDValRef NCDVal_NewString (NCDValMem *mem, const char *data);

/**
 * Builds a new StoredString.
 * Returns a reference to the new value, or an invalid reference
 * on out of memory.
 * WARNING: The buffer passed must NOT be part of any value in the
 * memory object specified. In particular, you may NOT use this
 * function to copy a string that resides in the same memory object.
 * 
 * A StoredString is a kind of String which is represented directly in the
 * value memory object.
 */
NCDValRef NCDVal_NewStringBin (NCDValMem *mem, const uint8_t *data, size_t len);

/**
 * See NCDVal_NewStringBin.
 */
NCDValRef NCDVal_NewStringBinMr (NCDValMem *mem, MemRef data);

/**
 * Builds a new StoredString of the given length with undefined contents.
 * You can define the contents of the string later by copying to the address
 * returned by {@link NCDVal_StringData}.
 */
NCDValRef NCDVal_NewStringUninitialized (NCDValMem *mem, size_t len);

/**
 * Builds a new IdString.
 * Returns a reference to the new value, or an invalid reference
 * on out of memory.
 * 
 * An IdString is a kind of String which is represented efficiently as a string
 * identifier via {@link NCDStringIndex}.
 */
NCDValRef NCDVal_NewIdString (NCDValMem *mem, NCD_string_id_t string_id);

/**
 * Builds a new ExternalString, pointing to the given external data. A reference to
 * the external data is taken using {@link BRefTarget}, unless 'ref_target' is
 * NULL. The data must not change while this value exists.
 * Returns a reference to the new value, or an invalid reference
 * on out of memory.
 * 
 * An ExternalString is a kind of String where the actual string contents are
 * stored outside of the value memory object.
 */
NCDValRef NCDVal_NewExternalString (NCDValMem *mem, const char *data, size_t len,
                                    BRefTarget *ref_target);

/**
 * Returns a pointer to the data of a String.
 * WARNING: the string data may not be null-terminated. To get a null-terminated
 * version, use {@link NCDVal_StringNullTerminate}.
 * The value reference must point to a String.
 * WARNING: the returned pointer may become invalid when any new value is inserted
 * into the residing memory object (due to a realloc of the value memory).
 */
const char * NCDVal_StringData (NCDValRef string);

/**
 * Returns the length of a String.
 * The value reference must point to a String.
 */
size_t NCDVal_StringLength (NCDValRef string);

/**
 * Returns a MemRef interface to the given string value.
 * WARNING: the returned pointer may become invalid when any new value is inserted
 * into the residing memory object (due to a realloc of the value memory).
 */
MemRef NCDVal_StringMemRef (NCDValRef string);

/**
 * Produces a null-terminated version of a String. On success, the result is
 * stored into an {@link NCDValNullTermString} structure, and the null-terminated
 * string is available via its 'data' member. This function may either simply pass
 * through the data pointer (if the string is known to be null-terminated) or
 * produce a null-terminated dynamically allocated copy.
 * On success, {@link NCDValNullTermString_Free} should be called to release any allocated
 * memory when the null-terminated string is no longer needed. This must be called before
 * the memory object is freed, because it may point to data inside the memory object.
 * It is guaranteed that *out is not modified on failure.
 * Returns 1 on success and 0 on failure.
 */
int NCDVal_StringNullTerminate (NCDValRef string, NCDValNullTermString *out) WARN_UNUSED;

/**
 * Returns a dummy {@link NCDValNullTermString} which can be freed using
 * {@link NCDValNullTermString_Free}, but need not be.
 */
NCDValNullTermString NCDValNullTermString_NewDummy (void);

/**
 * Releases any memory which was dynamically allocated by {@link NCDVal_StringNullTerminate}
 * to null-terminate a string.
 */
void NCDValNullTermString_Free (NCDValNullTermString *o);

/**
 * Returns the string ID of an IdString.
 */
NCD_string_id_t NCDVal_IdStringId (NCDValRef idstring);

/**
 * Returns the reference target of an ExternalString. This may be NULL
 * if the external string is not associated with a reference target.
 */
BRefTarget * NCDVal_ExternalStringTarget (NCDValRef externalstring);

/**
 * Determines if the String has any null bytes in its contents.
 */
int NCDVal_StringHasNulls (NCDValRef string);

/**
 * Determines if the String value is equal to the given null-terminated
 * string.
 * The value reference must point to a String value.
 */
int NCDVal_StringEquals (NCDValRef string, const char *data);

/**
 * Determines if the String is equal to the given string represented
 * by an {@link NCDStringIndex} identifier.
 */
int NCDVal_StringEqualsId (NCDValRef string, NCD_string_id_t string_id);

/**
 * Compares two String's in a manner similar to memcmp().
 * The startN and length arguments must refer to a valid region within
 * stringN, i.e. startN + length <= length_of_stringN must hold.
 */
int NCDVal_StringMemCmp (NCDValRef string1, NCDValRef string2, size_t start1, size_t start2, size_t length);

/**
 * Copies a part of a String to a buffer.
 * \a start and \a length must refer to a valid region within the string,
 * i.e. start + length <= length_of_string must hold.
 */
void NCDVal_StringCopyOut (NCDValRef string, size_t start, size_t length, char *dst);

/**
 * Determines if a part of a String is equal to the \a length bytes in \a data.
 * \a start and \a length must refer to a valid region within the string,
 * i.e. start + length <= length_of_string must hold.
 */
int NCDVal_StringRegionEquals (NCDValRef string, size_t start, size_t length, const char *data);

/**
 * Determines if a value is a list value.
 * The value reference must not be an invalid reference.
 */
int NCDVal_IsList (NCDValRef val);

/**
 * Builds a new list value. The 'maxcount' argument specifies how
 * many element slots to preallocate. Not more than that many
 * elements may be appended to the list using {@link NCDVal_ListAppend}.
 * Returns a reference to the new value, or an invalid reference
 * on out of memory.
 */
NCDValRef NCDVal_NewList (NCDValMem *mem, size_t maxcount);

/**
 * Appends a value to to the list value.
 * The 'list' reference must point to a list value, and the
 * 'elem' reference must be non-invalid and point to a value within
 * the same memory object as the list.
 * Inserting a value into a list does not in any way change it;
 * internally, the list only points to it.
 * You must not modify the element after it has been inserted into the
 * list.
 * Returns 1 on success and 0 on failure (depth limit exceeded).
 */
int NCDVal_ListAppend (NCDValRef list, NCDValRef elem) WARN_UNUSED;

/**
 * Returns the number of elements in a list value, i.e. the number
 * of times {@link NCDVal_ListAppend} was called.
 * The 'list' reference must point to a list value.
 */
size_t NCDVal_ListCount (NCDValRef list);

/**
 * Returns the maximum number of elements a list value may contain,
 * i.e. the 'maxcount' argument to {@link NCDVal_NewList}.
 * The 'list' reference must point to a list value.
 */
size_t NCDVal_ListMaxCount (NCDValRef list);

/**
 * Returns a reference to the value at the given position 'pos' in a list,
 * starting with zero.
 * The 'list' reference must point to a list value.
 * The position 'pos' must refer to an existing element, i.e.
 * pos < NCDVal_ListCount().
 */
NCDValRef NCDVal_ListGet (NCDValRef list, size_t pos);

/**
 * Returns references to elements within a list by writing them
 * via (NCDValRef *) variable arguments.
 * If 'num' == NCDVal_ListCount(), succeeds, returing 1 and writing 'num'
 * references, as mentioned.
 * If 'num' != NCDVal_ListCount(), fails, returning 0, without writing any
 * references
 */
int NCDVal_ListRead (NCDValRef list, int num, ...);

/**
 * Like NCDVal_ListRead but ignores the initial 'start' arguments,
 * that is, reads 'num' arguments after 'start'.
 * The 'start' must be <= the length of the list.
 */
int NCDVal_ListReadStart (NCDValRef list, int start, int num, ...);

/**
 * Like {@link NCDVal_ListRead}, but the list can contain more than 'num'
 * elements.
 */
int NCDVal_ListReadHead (NCDValRef list, int num, ...);

/**
 * Determines if a value is a map value.
 * The value reference must not be an invalid reference.
 */
int NCDVal_IsMap (NCDValRef val);

/**
 * Builds a new map value. The 'maxcount' argument specifies how
 * many entry slots to preallocate. Not more than that many
 * entries may be inserted to the map using {@link NCDVal_MapInsert}.
 * Returns a reference to the new value, or an invalid reference
 * on out of memory.
 */
NCDValRef NCDVal_NewMap (NCDValMem *mem, size_t maxcount);

/**
 * Inserts an entry to the map value.
 * The 'map' reference must point to a map value, and the
 * 'key' and 'val' references must be non-invalid and point to values within
 * the same memory object as the map.
 * Inserting an entry does not in any way change the 'key'and 'val';
 * internally, the map only points to it.
 * You must not modify the key after inserting it into a map. This is because
 * the map builds an embedded AVL tree of entries indexed by keys.
 * If insertion fails due to a maximum depth limit, returns 0.
 * Otherwise returns 1, and *out_inserted is set to 1 if the key did not
 * yet exist and the entry was inserted, and to 0 if it did exist and the
 * entry was not inserted. The 'out_inserted' pointer may be NULL, in which
 * case *out_inserted is never set.
 */
int NCDVal_MapInsert (NCDValRef map, NCDValRef key, NCDValRef val, int *out_inserted) WARN_UNUSED;

/**
 * Returns the number of entries in a map value, i.e. the number
 * of times {@link NCDVal_MapInsert} was called successfully.
 * The 'map' reference must point to a map value.
 */
size_t NCDVal_MapCount (NCDValRef map);

/**
 * Returns the maximum number of entries a map value may contain,
 * i.e. the 'maxcount' argument to {@link NCDVal_NewMap}.
 * The 'map' reference must point to a map value.
 */
size_t NCDVal_MapMaxCount (NCDValRef map);

/**
 * Determines if a map entry reference is invalid. This is used in combination
 * with the map iteration functions to detect the end of iteration.
 */
int NCDVal_MapElemInvalid (NCDValMapElem me);

/**
 * Returns a reference to the first entry in a map, with respect to some
 * arbitrary order.
 * If the map is empty, returns an invalid map entry reference.
 */
NCDValMapElem NCDVal_MapFirst (NCDValRef map);

/**
 * Returns a reference to the entry in a map that follows the entry referenced
 * by 'me', with respect to some arbitrary order.
 * The 'me' argument must be a non-invalid reference to an entry in the map.
 * If 'me' is the last entry, returns an invalid map entry reference.
 */
NCDValMapElem NCDVal_MapNext (NCDValRef map, NCDValMapElem me);

/**
 * Like {@link NCDVal_MapFirst}, but with respect to the order defined by
 * {@link NCDVal_Compare}.
 * Ordered iteration is slower and should only be used when needed.
 */
NCDValMapElem NCDVal_MapOrderedFirst (NCDValRef map);

/**
 * Like {@link NCDVal_MapNext}, but with respect to the order defined by
 * {@link NCDVal_Compare}.
 * Ordered iteration is slower and should only be used when needed.
 */
NCDValMapElem NCDVal_MapOrderedNext (NCDValRef map, NCDValMapElem me);

/**
 * Returns a reference to the key of the map entry referenced by 'me'.
 * The 'me' argument must be a non-invalid reference to an entry in the map.
 */
NCDValRef NCDVal_MapElemKey (NCDValRef map, NCDValMapElem me);

/**
 * Returns a reference to the value of the map entry referenced by 'me'.
 * The 'me' argument must be a non-invalid reference to an entry in the map.
 */
NCDValRef NCDVal_MapElemVal (NCDValRef map, NCDValMapElem me);

/**
 * Looks for a key in the map. The 'key' reference must be a non-invalid
 * value reference, and may point to a value in a different memory object
 * than the map.
 * If the key exists in the map, returns a reference to the corresponding
 * map entry.
 * If the key does not exist, returns an invalid map entry reference.
 */
NCDValMapElem NCDVal_MapFindKey (NCDValRef map, NCDValRef key);

/**
 * Retrieves the value reference to the value of the map entry whose key is a
 * string value equal to the given null-terminated string. If there is no such
 * entry, returns an invalid value reference.
 */
NCDValRef NCDVal_MapGetValue (NCDValRef map, const char *key_str);

/**
 * Builds a placeholder replacement program, which is a list of instructions for
 * efficiently replacing placeholders in identical values in identical memory
 * objects.
 * To actually perform replacements, make copies of the memory object of this value
 * using {@link NCDValMem_InitCopy}, then call {@link NCDValReplaceProg_Execute}
 * on the copies.
 * The value passed must be a valid value, and not a placeholder.
 * Returns 1 on success, 0 on failure.
 */
int NCDValReplaceProg_Init (NCDValReplaceProg *o, NCDValRef val);

/**
 * Frees the placeholder replacement program.
 */
void NCDValReplaceProg_Free (NCDValReplaceProg *o);

/**
 * Callback used by {@link NCDValReplaceProg_Execute} to allow the caller to produce
 * values of placeholders.
 * This function should build a new value within the memory object 'mem' (which is
 * the same as of the memory object where placeholders are being replaced).
 * On success, it should return 1, writing a valid value reference to *out.
 * On failure, it can either return 0, or return 1 but write an invalid value reference.
 * This callback must not access the memory object in any other way than building
 * new values in it; it must not modify any values that were already present at the
 * point it was called.
 */
typedef int (*NCDVal_replace_func) (void *arg, int plid, NCDValMem *mem, NCDValRef *out);

/**
 * Executes the replacement program, replacing placeholders in a value.
 * The memory object must given be identical to the memory object which was used in
 * {@link NCDValReplaceProg_Init}; see {@link NCDValMem_InitCopy}.
 * This will call the callback 'replace', which should build the values to replace
 * the placeholders.
 * Returns 1 on success and 0 on failure. On failure, the entire memory object enters
 * and inconsistent state and must be freed using {@link NCDValMem_Free} before
 * performing any other operation on it.
 * The program is passed by value instead of pointer because this appears to be faster.
 * Is is not modified in any way.
 */
int NCDValReplaceProg_Execute (NCDValReplaceProg prog, NCDValMem *mem, NCDVal_replace_func replace, void *arg);

#endif
