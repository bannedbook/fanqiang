/**
 * @file CAvl_footer.h
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

#undef CAVL_PARAM_NAME
#undef CAVL_PARAM_FEATURE_COUNTS
#undef CAVL_PARAM_FEATURE_KEYS_ARE_INDICES
#undef CAVL_PARAM_FEATURE_NOKEYS
#undef CAVL_PARAM_FEATURE_ASSOC
#undef CAVL_PARAM_TYPE_ENTRY
#undef CAVL_PARAM_TYPE_LINK
#undef CAVL_PARAM_TYPE_KEY
#undef CAVL_PARAM_TYPE_ARG
#undef CAVL_PARAM_TYPE_COUNT
#undef CAVL_PARAM_TYPE_ASSOC
#undef CAVL_PARAM_VALUE_COUNT_MAX
#undef CAVL_PARAM_VALUE_NULL
#undef CAVL_PARAM_VALUE_ASSOC_ZERO
#undef CAVL_PARAM_FUN_DEREF
#undef CAVL_PARAM_FUN_COMPARE_ENTRIES
#undef CAVL_PARAM_FUN_COMPARE_KEY_ENTRY
#undef CAVL_PARAM_FUN_ASSOC_VALUE
#undef CAVL_PARAM_FUN_ASSOC_OPER
#undef CAVL_PARAM_MEMBER_CHILD
#undef CAVL_PARAM_MEMBER_BALANCE
#undef CAVL_PARAM_MEMBER_PARENT
#undef CAVL_PARAM_MEMBER_COUNT
#undef CAVL_PARAM_MEMBER_ASSOC

#undef CAvl
#undef CAvlEntry
#undef CAvlLink
#undef CAvlRef
#undef CAvlArg
#undef CAvlKey
#undef CAvlCount
#undef CAvlAssoc

#undef CAvlIsNullRef
#undef CAvlIsValidRef
#undef CAvlDeref

#undef CAvl_Init
#undef CAvl_Insert
#undef CAvl_InsertAt
#undef CAvl_Remove
#undef CAvl_Lookup
#undef CAvl_LookupExact
#undef CAvl_GetFirstGreater
#undef CAvl_GetLastLesser
#undef CAvl_GetFirstGreaterEqual
#undef CAvl_GetLastLesserEqual
#undef CAvl_GetFirst
#undef CAvl_GetLast
#undef CAvl_GetNext
#undef CAvl_GetPrev
#undef CAvl_IsEmpty
#undef CAvl_Verify
#undef CAvl_Count
#undef CAvl_IndexOf
#undef CAvl_GetAt
#undef CAvl_AssocSum
#undef CAvl_ExclusiveAssocPrefixSum
#undef CAvl_FindLastExclusiveAssocPrefixSumLesserEqual

#undef CAvl_link
#undef CAvl_balance
#undef CAvl_parent
#undef CAvl_count
#undef CAvl_assoc
#undef CAvl_nulllink
#undef CAvl_nullref
#undef CAvl_compare_entries
#undef CAvl_compare_key_entry
#undef CAvl_compute_node_assoc
#undef CAvl_check_parent
#undef CAvl_verify_recurser
#undef CAvl_assert_tree
#undef CAvl_update_count_from_children
#undef CAvl_rotate
#undef CAvl_subtree_min
#undef CAvl_subtree_max
#undef CAvl_replace_subtree_fix_assoc
#undef CAvl_swap_for_remove
#undef CAvl_rebalance
#undef CAvl_child_count
#undef CAvl_MAX
#undef CAvl_OPTNEG
