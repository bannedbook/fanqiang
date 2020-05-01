/**
 * @file SLinkedList_header.h
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
// SLINKEDLIST_PARAM_NAME - name of this data structure
// SLINKEDLIST_PARAM_FEATURE_LAST - whether to keep track of the last entry in the list (0/1)
// SLINKEDLIST_PARAM_TYPE_ENTRY - type of entry
// SLINKEDLIST_PARAM_MEMBER_NODE - name of the node member in entry

// types
#define SLinkedList SLINKEDLIST_PARAM_NAME
#define SLinkedListEntry SLINKEDLIST_PARAM_TYPE_ENTRY
#define SLinkedListNode MERGE(SLinkedList, Node)

// non-object public functions
#define SLinkedListMarkRemoved MERGE(SLinkedList, MarkRemoved)
#define SLinkedListIsRemoved MERGE(SLinkedList, IsRemoved)

// public functions
#define SLinkedList_Init MERGE(SLinkedList, _Init)
#define SLinkedList_Next MERGE(SLinkedList, _Next)
#define SLinkedList_Prev MERGE(SLinkedList, _Prev)
#define SLinkedList_Prepend MERGE(SLinkedList, _Prepend)
#define SLinkedList_Append MERGE(SLinkedList, _Append)
#define SLinkedList_InsertBefore MERGE(SLinkedList, _InsertBefore)
#define SLinkedList_InsertAfter MERGE(SLinkedList, _InsertAfter)
#define SLinkedList_Remove MERGE(SLinkedList, _Remove)
#define SLinkedList_RemoveFirst MERGE(SLinkedList, _RemoveFirst)
#define SLinkedList_RemoveLast MERGE(SLinkedList, _RemoveLast)
#define SLinkedList_First MERGE(SLinkedList, _First)
#define SLinkedList_Last MERGE(SLinkedList, _Last)
#define SLinkedList_IsEmpty MERGE(SLinkedList, _IsEmpty)

// private things
#define SLinkedList_prev(entry) ((entry)->SLINKEDLIST_PARAM_MEMBER_NODE . prev)
#define SLinkedList_next(entry) ((entry)->SLINKEDLIST_PARAM_MEMBER_NODE . next)
