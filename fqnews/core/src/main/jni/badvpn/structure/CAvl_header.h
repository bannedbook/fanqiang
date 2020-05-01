/**
 * @file CAvl_header.h
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
// CAVL_PARAM_NAME - name of this data structure
// CAVL_PARAM_FEATURE_COUNTS - whether to keep count information (0 or 1)
// CAVL_PARAM_FEATURE_KEYS_ARE_INDICES - (0 or 1) whether to assume the keys are entry indices
//   (number of entries lesser than given entry). If yes, CAVL_PARAM_TYPE_KEY is unused.
//   Requires CAVL_PARAM_FEATURE_COUNTS.
// CAVL_PARAM_FEATURE_NOKEYS - define to 1 if there is no need for a lookup operation
// CAVL_PARAM_FEATURE_ASSOC - define to 1 for computation of an associative operation on subtrees.
//   If enabled, the following macros must be defined: CAVL_PARAM_TYPE_ASSOC,
//   CAVL_PARAM_VALUE_ASSOC_ZERO, CAVL_PARAM_FUN_ASSOC_VALUE,
//   CAVL_PARAM_FUN_ASSOC_OPER, CAVL_PARAM_MEMBER_ASSOC.
// CAVL_PARAM_TYPE_ENTRY - type of entry
// CAVL_PARAM_TYPE_LINK - type of entry link (usually pointer or index)
// CAVL_PARAM_TYPE_KEY - type of key (only if not CAVL_PARAM_FEATURE_KEYS_ARE_INDICES and
//   not CAVL_PARAM_FEATURE_NOKEYS)
// CAVL_PARAM_TYPE_ARG - type of argument pass through to callbacks
// CAVL_PARAM_TYPE_COUNT - type of count (only if CAVL_PARAM_FEATURE_COUNTS)
// CAVL_PARAM_TYPE_ASSOC - type of associative operation result
// CAVL_PARAM_VALUE_COUNT_MAX - maximum value of count (type is CAVL_PARAM_TYPE_COUNT)
// CAVL_PARAM_VALUE_NULL - value of invalid link (type is CAVL_PARAM_TYPE_LINK)
// CAVL_PARAM_VALUE_ASSOC_ZERO - zero value for associative operation (type is CAVL_PARAM_TYPE_ASSOC).
//   This must be both a left- and right-identity for the associative operation.
// CAVL_PARAM_FUN_DEREF(arg, link) - dereference a non-null link; returns pointer to CAVL_PARAM_TYPE_LINK
// CAVL_PARAM_FUN_COMPARE_ENTRIES(arg, entry1, entry2) - compare to entries; returns -1/0/1
// CAVL_PARAM_FUN_COMPARE_KEY_ENTRY(arg, key1, entry2) - compare key and entry; returns -1/0/1
// CAVL_PARAM_FUN_ASSOC_VALUE(arg, entry) - get value of a node for associative operation.
//   The result will be cast to CAVL_PARAM_TYPE_ASSOC.
// CAVL_PARAM_FUN_ASSOC_OPER(arg, value1, value2) - compute the associative operation on two values.
//   The type of the two values is CAVL_PARAM_TYPE_ASSOC, and the result will be cast to
//   CAVL_PARAM_TYPE_ASSOC.
// CAVL_PARAM_MEMBER_CHILD - name of the child member in entry (type is CAVL_PARAM_TYPE_LINK[2])
// CAVL_PARAM_MEMBER_BALANCE - name of the balance member in entry (type is any signed integer)
// CAVL_PARAM_MEMBER_PARENT - name of the parent member in entry (type is CAVL_PARAM_TYPE_LINK)
// CAVL_PARAM_MEMBER_COUNT - name of the count member in entry (type is CAVL_PARAM_TYPE_COUNT)
//   (only if CAVL_PARAM_FEATURE_COUNTS)
// CAVL_PARAM_MEMBER_ASSOC - name of assoc member in entry (type is CAVL_PARAM_TYPE_ASSOC)

#ifndef BADVPN_CAVL_H
#error CAvl.h has not been included
#endif

#if CAVL_PARAM_FEATURE_KEYS_ARE_INDICES && !CAVL_PARAM_FEATURE_COUNTS
#error CAVL_PARAM_FEATURE_KEYS_ARE_INDICES requires CAVL_PARAM_FEATURE_COUNTS
#endif

#if CAVL_PARAM_FEATURE_KEYS_ARE_INDICES && CAVL_PARAM_FEATURE_NOKEYS
#error CAVL_PARAM_FEATURE_KEYS_ARE_INDICES and CAVL_PARAM_FEATURE_NOKEYS cannot be used together
#endif

// types
#define CAvl CAVL_PARAM_NAME
#define CAvlEntry CAVL_PARAM_TYPE_ENTRY
#define CAvlLink CAVL_PARAM_TYPE_LINK
#define CAvlRef MERGE(CAVL_PARAM_NAME, Ref)
#define CAvlArg CAVL_PARAM_TYPE_ARG
#define CAvlKey CAVL_PARAM_TYPE_KEY
#define CAvlCount CAVL_PARAM_TYPE_COUNT
#define CAvlAssoc CAVL_PARAM_TYPE_ASSOC

// non-object public functions
#define CAvlIsNullRef MERGE(CAvl, IsNullRef)
#define CAvlIsValidRef MERGE(CAvl, IsValidRef)
#define CAvlDeref MERGE(CAvl, Deref)

// public functions
#define CAvl_Init MERGE(CAvl, _Init)
#define CAvl_Insert MERGE(CAvl, _Insert)
#define CAvl_InsertAt MERGE(CAvl, _InsertAt)
#define CAvl_Remove MERGE(CAvl, _Remove)
#define CAvl_Lookup MERGE(CAvl, _Lookup)
#define CAvl_LookupExact MERGE(CAvl, _LookupExact)
#define CAvl_GetFirstGreater MERGE(CAvl, _GetFirstGreater)
#define CAvl_GetLastLesser MERGE(CAvl, _GetLastLesser)
#define CAvl_GetFirstGreaterEqual MERGE(CAvl, _GetFirstGreaterEqual)
#define CAvl_GetLastLesserEqual MERGE(CAvl, _GetLastLesserEqual)
#define CAvl_GetFirst MERGE(CAvl, _GetFirst)
#define CAvl_GetLast MERGE(CAvl, _GetLast)
#define CAvl_GetNext MERGE(CAvl, _GetNext)
#define CAvl_GetPrev MERGE(CAvl, _GetPrev)
#define CAvl_IsEmpty MERGE(CAvl, _IsEmpty)
#define CAvl_Verify MERGE(CAvl, _Verify)
#define CAvl_Count MERGE(CAvl, _Count)
#define CAvl_IndexOf MERGE(CAvl, _IndexOf)
#define CAvl_GetAt MERGE(CAvl, _GetAt)
#define CAvl_AssocSum MERGE(CAvl, _AssocSum)
#define CAvl_ExclusiveAssocPrefixSum MERGE(CAvl, _ExclusiveAssocPrefixSum)
#define CAvl_FindLastExclusiveAssocPrefixSumLesserEqual MERGE(CAvl, _FindLastExclusiveAssocPrefixSumLesserEqual)

// private stuff
#define CAvl_link(entry) ((entry).ptr->CAVL_PARAM_MEMBER_CHILD)
#define CAvl_balance(entry) ((entry).ptr->CAVL_PARAM_MEMBER_BALANCE)
#define CAvl_parent(entry) ((entry).ptr->CAVL_PARAM_MEMBER_PARENT)
#define CAvl_count(entry) ((entry).ptr->CAVL_PARAM_MEMBER_COUNT)
#define CAvl_assoc(entry) ((entry).ptr->CAVL_PARAM_MEMBER_ASSOC)
#define CAvl_nulllink MERGE(CAvl, __nulllink)
#define CAvl_nullref MERGE(CAvl, __nullref)
#define CAvl_compare_entries MERGE(CAVL_PARAM_NAME, _compare_entries)
#define CAvl_compare_key_entry MERGE(CAVL_PARAM_NAME, _compare_key_entry)
#define CAvl_compute_node_assoc MERGE(CAVL_PARAM_NAME, _compute_node_assoc)
#define CAvl_check_parent MERGE(CAVL_PARAM_NAME, _check_parent)
#define CAvl_verify_recurser MERGE(CAVL_PARAM_NAME, _verify_recurser)
#define CAvl_assert_tree MERGE(CAVL_PARAM_NAME, _assert_tree)
#define CAvl_update_count_from_children MERGE(CAVL_PARAM_NAME, _update_count_from_children)
#define CAvl_rotate MERGE(CAVL_PARAM_NAME, _rotate)
#define CAvl_subtree_min MERGE(CAVL_PARAM_NAME, _subtree_min)
#define CAvl_subtree_max MERGE(CAVL_PARAM_NAME, _subtree_max)
#define CAvl_replace_subtree_fix_assoc MERGE(CAVL_PARAM_NAME, _replace_subtree_fix_counts)
#define CAvl_swap_for_remove MERGE(CAVL_PARAM_NAME, _swap_entries)
#define CAvl_rebalance MERGE(CAVL_PARAM_NAME, _rebalance)
#define CAvl_child_count MERGE(CAvl, __child_count)
#define CAvl_MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#define CAvl_OPTNEG(_a, _neg) ((_neg) ? -(_a) : (_a))
