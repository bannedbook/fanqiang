/**
 * @file IndexedList.h
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
 * 
 * @section DESCRIPTION
 * 
 * A data structure similar to a list, but with efficient index-based access.
 */

#ifndef BADVPN_INDEXEDLIST_H
#define BADVPN_INDEXEDLIST_H

#include <stddef.h>
#include <stdint.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <structure/CAvl.h>

typedef struct IndexedList_s IndexedList;
typedef struct IndexedListNode_s IndexedListNode;

typedef IndexedListNode *IndexedList__tree_link;

#include "IndexedList_tree.h"
#include <structure/CAvl_decl.h>

struct IndexedList_s {
    IndexedList__Tree tree;
};

struct IndexedListNode_s {
    IndexedListNode *tree_child[2];
    IndexedListNode *tree_parent;
    int8_t tree_balance;
    uint64_t tree_count;
};

/**
 * Initializes the indexed list.
 * 
 * @param o uninitialized list object to initialize
 */
static void IndexedList_Init (IndexedList *o);

/**
 * Inserts a node into the indexed list.
 * 
 * @param o indexed list to insert into
 * @param node uninitialized node to insert
 * @param index index to insert at (starting with zero). Any existing elements
 *              at or after this index will be shifted forward, i.e. their
 *              indices will be incremented by one. Must be <=count.
 */
static void IndexedList_InsertAt (IndexedList *o, IndexedListNode *node, uint64_t index);

/**
 * Removes a nove from the indexed list.
 * 
 * @param o indexed list to remove from
 * @param node node in the list to remove
 */
static void IndexedList_Remove (IndexedList *o, IndexedListNode *node);

/**
 * Returns the number of nodes in the indexed list.
 * 
 * @param o indexed list
 * @return number of nodes
 */
static uint64_t IndexedList_Count (IndexedList *o);

/**
 * Returns the index of a node in the indexed list.
 * 
 * @param o indexed list
 * @param node node in the list to get index of
 * @return index of the node
 */
static uint64_t IndexedList_IndexOf (IndexedList *o, IndexedListNode *node);

/**
 * Returns the node at the specified index in the indexed list.
 * 
 * @param o indexed list
 * @param index index of the node to return. Must be < count.
 * @return node at the specified index
 */
static IndexedListNode * IndexedList_GetAt (IndexedList *o, uint64_t index);

/**
 * Returns the first node, or NULL if the list is empty.
 * 
 * @param o indexed list
 * @return first node, or NULL
 */
static IndexedListNode * IndexedList_GetFirst (IndexedList *o);

/**
 * Returns the last node, or NULL if the list is empty.
 * 
 * @param o indexed list
 * @return last node, or NULL
 */
static IndexedListNode * IndexedList_GetLast (IndexedList *o);

/**
 * Returns the next node of a given node, or NULL this is the last node.
 * 
 * @param o indexed list
 * @param node existing node
 * @return next node, or NULL
 */
static IndexedListNode * IndexedList_GetNext (IndexedList *o, IndexedListNode *node);

/**
 * Returns the previous node of a given node, or NULL this is the first node.
 * 
 * @param o indexed list
 * @param node existing node
 * @return previous node, or NULL
 */
static IndexedListNode * IndexedList_GetPrev (IndexedList *o, IndexedListNode *node);

#include "IndexedList_tree.h"
#include <structure/CAvl_impl.h>

static IndexedListNode * IndexedList__deref (IndexedList__TreeRef ref)
{
    return ref.link;
}

static void IndexedList_Init (IndexedList *o)
{
    IndexedList__Tree_Init(&o->tree);
}

static void IndexedList_InsertAt (IndexedList *o, IndexedListNode *node, uint64_t index)
{
    ASSERT(index <= IndexedList__Tree_Count(&o->tree, 0))
    ASSERT(IndexedList__Tree_Count(&o->tree, 0) < UINT64_MAX - 1)
    
    uint64_t orig_count = IndexedList__Tree_Count(&o->tree, 0);
    B_USE(orig_count)
    
    IndexedList__Tree_InsertAt(&o->tree, 0, IndexedList__TreeDeref(0, node), index);
    
    ASSERT(IndexedList__Tree_IndexOf(&o->tree, 0, IndexedList__TreeDeref(0, node)) == index)
    ASSERT(IndexedList__Tree_Count(&o->tree, 0) == orig_count + 1)
}

static void IndexedList_Remove (IndexedList *o, IndexedListNode *node)
{
    IndexedList__Tree_Remove(&o->tree, 0, IndexedList__TreeDeref(0, node));
}

static uint64_t IndexedList_Count (IndexedList *o)
{
    return IndexedList__Tree_Count(&o->tree, 0);
}

static uint64_t IndexedList_IndexOf (IndexedList *o, IndexedListNode *node)
{
    return IndexedList__Tree_IndexOf(&o->tree, 0, IndexedList__TreeDeref(0, node));
}

static IndexedListNode * IndexedList_GetAt (IndexedList *o, uint64_t index)
{
    ASSERT(index < IndexedList__Tree_Count(&o->tree, 0))
    
    IndexedList__TreeRef ref = IndexedList__Tree_GetAt(&o->tree, 0, index);
    ASSERT(!IndexedList__TreeIsNullRef(ref))
    
    return ref.ptr;
}

static IndexedListNode * IndexedList_GetFirst (IndexedList *o)
{
    return IndexedList__deref(IndexedList__Tree_GetFirst(&o->tree, 0));
}

static IndexedListNode * IndexedList_GetLast (IndexedList *o)
{
    return IndexedList__deref(IndexedList__Tree_GetLast(&o->tree, 0));
}

static IndexedListNode * IndexedList_GetNext (IndexedList *o, IndexedListNode *node)
{
    ASSERT(node)
    
    return IndexedList__deref(IndexedList__Tree_GetNext(&o->tree, 0, IndexedList__TreeDeref(0, node)));
}

static IndexedListNode * IndexedList_GetPrev (IndexedList *o, IndexedListNode *node)
{
    ASSERT(node)
    
    return IndexedList__deref(IndexedList__Tree_GetPrev(&o->tree, 0, IndexedList__TreeDeref(0, node)));
}

#endif
