/**
 * @file SLinkedList_decl.h
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

#include "SLinkedList_header.h"

typedef struct {
    SLinkedListEntry *first;
#if SLINKEDLIST_PARAM_FEATURE_LAST
    SLinkedListEntry *last;
#endif
} SLinkedList;

typedef struct {
    SLinkedListEntry *prev;
    SLinkedListEntry *next;
} SLinkedListNode;

static void SLinkedListMarkRemoved (SLinkedListEntry *entry);
static int SLinkedListIsRemoved (SLinkedListEntry *entry);

static void SLinkedList_Init (SLinkedList *o);
static SLinkedListEntry * SLinkedList_Next (SLinkedList *o, SLinkedListEntry *entry);
static SLinkedListEntry * SLinkedList_Prev (SLinkedList *o, SLinkedListEntry *entry);
static void SLinkedList_Prepend (SLinkedList *o, SLinkedListEntry *entry);
#if SLINKEDLIST_PARAM_FEATURE_LAST
static void SLinkedList_Append (SLinkedList *o, SLinkedListEntry *entry);
#endif
static void SLinkedList_InsertBefore (SLinkedList *o, SLinkedListEntry *entry, SLinkedListEntry *before_entry);
static void SLinkedList_InsertAfter (SLinkedList *o, SLinkedListEntry *entry, SLinkedListEntry *after_entry);
static void SLinkedList_Remove (SLinkedList *o, SLinkedListEntry *entry);
static void SLinkedList_RemoveFirst (SLinkedList *o);
#if SLINKEDLIST_PARAM_FEATURE_LAST
static void SLinkedList_RemoveLast (SLinkedList *o);
#endif
static SLinkedListEntry * SLinkedList_First (const SLinkedList *o);
#if SLINKEDLIST_PARAM_FEATURE_LAST
static SLinkedListEntry * SLinkedList_Last (const SLinkedList *o);
#endif
static int SLinkedList_IsEmpty (const SLinkedList *o);

#include "SLinkedList_footer.h"
