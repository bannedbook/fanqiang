/**
 * @file LinkedList3.h
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
 * Doubly linked list that support multiple iterations and removing
 * aritrary elements during iteration, without a central object to
 * represent the list.
 */

#ifndef BADVPN_STRUCTURE_LINKEDLIST3_H
#define BADVPN_STRUCTURE_LINKEDLIST3_H

#include <stddef.h>
#include <stdint.h>

#include <misc/debug.h>

struct _LinkedList3Iterator;

/**
 * Linked list node.
 */
typedef struct _LinkedList3Node {
    struct _LinkedList3Node *p;
    struct _LinkedList3Node *n;
    struct _LinkedList3Iterator *it;
} LinkedList3Node;

/**
 * Linked list iterator.
 */
typedef struct _LinkedList3Iterator {
    int8_t dir;
    struct _LinkedList3Node *e;
    struct _LinkedList3Iterator *pi;
    struct _LinkedList3Iterator *ni;
} LinkedList3Iterator;

/**
 * Initializes a list node to form a new list consisting of a
 * single node.
 * 
 * @param node list node structure to initialize. The node must remain
 *        available until it is freed with {@link LinkedList3Node_Free},
 *        or the list is no longer required.
 */
static void LinkedList3Node_InitLonely (LinkedList3Node *node);

/**
 * Initializes a list node to go after an existing node.
 * 
 * @param node list node structure to initialize. The node must remain
 *        available until it is freed with {@link LinkedList3Node_Free},
 *        or the list is no longer required.
 * @param ref existing list node
 */
static void LinkedList3Node_InitAfter (LinkedList3Node *node, LinkedList3Node *ref);

/**
 * Initializes a list node to go before an existing node.
 * 
 * @param node list node structure to initialize. The node must remain
 *        available until it is freed with {@link LinkedList3Node_Free},
 *        or the list is no longer required.
 * @param ref existing list node
 */
static void LinkedList3Node_InitBefore (LinkedList3Node *node, LinkedList3Node *ref);

/**
 * Frees a list node, removing it a list (if there were other nodes
 * in the list).
 * 
 * @param node list node to free
 */
static void LinkedList3Node_Free (LinkedList3Node *node);

/**
 * Determines if a list node is a single node in a list.
 * 
 * @param node list node
 * @return 1 if the node ia a single node, 0 if not
 */
static int LinkedList3Node_IsLonely (LinkedList3Node *node);

/**
 * Returnes the node preceding this node (if there is one),
 * the node following this node (if there is one), or NULL,
 * respectively.
 * 
 * @param node list node
 * @return neighbour node or NULL if none
 */
static LinkedList3Node * LinkedList3Node_PrevOrNext (LinkedList3Node *node);

/**
 * Returnes the node following this node (if there is one),
 * the node preceding this node (if there is one), or NULL,
 * respectively.
 * 
 * @param node list node
 * @return neighbour node or NULL if none
 */
static LinkedList3Node * LinkedList3Node_NextOrPrev (LinkedList3Node *node);

/**
 * Returns the node preceding this node, or NULL if there is none.
 * 
 * @param node list node
 * @return left neighbour, or NULL if none
 */
static LinkedList3Node * LinkedList3Node_Prev (LinkedList3Node *node);

/**
 * Returns the node following this node, or NULL if there is none.
 * 
 * @param node list node
 * @return right neighbour, or NULL if none
 */
static LinkedList3Node * LinkedList3Node_Next (LinkedList3Node *node);

/**
 * Returns the first node in the list which this node is part of.
 * It is found by iterating the list from this node to the beginning.
 * 
 * @param node list node
 * @return first node in the list
 */
static LinkedList3Node * LinkedList3Node_First (LinkedList3Node *node);

/**
 * Returns the last node in the list which this node is part of.
 * It is found by iterating the list from this node to the end.
 * 
 * @param node list node
 * @return last node in the list
 */
static LinkedList3Node * LinkedList3Node_Last (LinkedList3Node *node);

/**
 * Initializes a linked list iterator.
 * The iterator structure must remain available until either of these occurs:
 *   - the list is no longer needed, or
 *   - the iterator is freed with {@link LinkedList3Iterator_Free}, or
 *   - the iterator reaches the end of iteration.
 * 
 * @param it uninitialized iterator to initialize
 * @param e initial position of the iterator. NULL for end of iteration.
 * @param dir direction of iteration. Must be 1 (forward) or -1 (backward).
 */
static void LinkedList3Iterator_Init (LinkedList3Iterator *it, LinkedList3Node *e, int dir);

/**
 * Frees a linked list iterator.
 * 
 * @param it iterator to free
 */
static void LinkedList3Iterator_Free (LinkedList3Iterator *it);

/**
 * Moves the iterator one node forward or backward (depending on its direction), or,
 * if it's at the last or first node (depending on the direction), it reaches
 * the end of iteration, or, if it's at the end of iteration, it remains there.
 * Returns the the previous position.
 * 
 * @param it the iterator
 * @return node on the position of iterator before it was (possibly) moved, or NULL
 *         if it was at the end of iteration
 */
static LinkedList3Node * LinkedList3Iterator_Next (LinkedList3Iterator *it);

void LinkedList3Node_InitLonely (LinkedList3Node *node)
{
    node->p = NULL;
    node->n = NULL;
    node->it = NULL;
}

void LinkedList3Node_InitAfter (LinkedList3Node *node, LinkedList3Node *ref)
{
    ASSERT(ref)
    
    node->p = ref;
    node->n = ref->n;
    ref->n = node;
    if (node->n) {
        node->n->p = node;
    }
    node->it = NULL;
}

void LinkedList3Node_InitBefore (LinkedList3Node *node, LinkedList3Node *ref)
{
    ASSERT(ref)
    
    node->n = ref;
    node->p = ref->p;
    ref->p = node;
    if (node->p) {
        node->p->n = node;
    }
    node->it = NULL;
}

void LinkedList3Node_Free (LinkedList3Node *node)
{
    // jump iterators
    while (node->it) {
        LinkedList3Iterator_Next(node->it);
    }
    
    if (node->p) {
        node->p->n = node->n;
    }
    if (node->n) {
        node->n->p = node->p;
    }
}

int LinkedList3Node_IsLonely (LinkedList3Node *node)
{
    return (!node->p && !node->n);
}

LinkedList3Node * LinkedList3Node_PrevOrNext (LinkedList3Node *node)
{
    if (node->p) {
        return node->p;
    }
    if (node->n) {
        return node->n;
    }
    return NULL;
}

LinkedList3Node * LinkedList3Node_NextOrPrev (LinkedList3Node *node)
{
    if (node->n) {
        return node->n;
    }
    if (node->p) {
        return node->p;
    }
    return NULL;
}

LinkedList3Node * LinkedList3Node_Prev (LinkedList3Node *node)
{
    return node->p;
}

LinkedList3Node * LinkedList3Node_Next (LinkedList3Node *node)
{
    return node->n;
}

LinkedList3Node * LinkedList3Node_First (LinkedList3Node *node)
{
    while (node->p) {
        node = node->p;
    }
    
    return node;
}

LinkedList3Node * LinkedList3Node_Last (LinkedList3Node *node)
{
    while (node->n) {
        node = node->n;
    }
    
    return node;
}

void LinkedList3Iterator_Init (LinkedList3Iterator *it, LinkedList3Node *e, int dir)
{
    ASSERT(dir == -1 || dir == 1)
    
    it->dir = dir;
    it->e = e;
    
    if (e) {
        // link into node's iterator list
        it->pi = NULL;
        it->ni = e->it;
        if (e->it) {
            e->it->pi = it;
        }
        e->it = it;
    }
}

void LinkedList3Iterator_Free (LinkedList3Iterator *it)
{
    if (it->e) {
        // remove from node's iterator list
        if (it->ni) {
            it->ni->pi = it->pi;
        }
        if (it->pi) {
            it->pi->ni = it->ni;
        } else {
            it->e->it = it->ni;
        }
    }
}

LinkedList3Node * LinkedList3Iterator_Next (LinkedList3Iterator *it)
{
    // remember original entry
    LinkedList3Node *orig = it->e;
    
    // jump to next entry
    if (it->e) {
        // get next entry
        LinkedList3Node *next = NULL; // to remove warning
        switch (it->dir) {
            case 1:
                next = it->e->n;
                break;
            case -1:
                next = it->e->p;
                break;
            default:
                ASSERT(0);
        }
        // destroy interator
        LinkedList3Iterator_Free(it);
        // re-initialize at next entry
        LinkedList3Iterator_Init(it, next, it->dir);
    }
    
    // return original entry
    return orig;
}

#endif
