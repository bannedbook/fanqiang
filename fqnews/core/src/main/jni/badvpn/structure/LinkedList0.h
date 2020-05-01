/**
 * @file LinkedList0.h
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
 * Very simple doubly linked list, with only a 'first' pointer an no 'last'
 * pointer.
 */

#ifndef BADVPN_STRUCTURE_LINKEDLIST0_H
#define BADVPN_STRUCTURE_LINKEDLIST0_H

#include <stddef.h>

#include <misc/debug.h>

/**
 * Linked list node.
 */
typedef struct LinkedList0Node_t
{
    struct LinkedList0Node_t *p;
    struct LinkedList0Node_t *n;
} LinkedList0Node;

/**
 * Simple doubly linked list.
 */
typedef struct
{
    LinkedList0Node *first;
} LinkedList0;

/**
 * Initializes the linked list.
 * 
 * @param list list to initialize
 */
static void LinkedList0_Init (LinkedList0 *list);

/**
 * Determines if the list is empty.
 * 
 * @param list the list
 * @return 1 if empty, 0 if not
 */
static int LinkedList0_IsEmpty (LinkedList0 *list);

/**
 * Returns the first node of the list.
 * 
 * @param list the list
 * @return first node of the list, or NULL if the list is empty
 */
static LinkedList0Node * LinkedList0_GetFirst (LinkedList0 *list);

/**
 * Inserts a node to the beginning of the list.
 * 
 * @param list the list
 * @param node uninitialized node to insert
 */
static void LinkedList0_Prepend (LinkedList0 *list, LinkedList0Node *node);

/**
 * Inserts a node before a given node.
 * 
 * @param list the list
 * @param node uninitialized node to insert
 * @param target node in the list to insert before
 */
static void LinkedList0_InsertBefore (LinkedList0 *list, LinkedList0Node *node, LinkedList0Node *target);

/**
 * Inserts a node after a given node.
 * 
 * @param list the list
 * @param node uninitialized node to insert
 * @param target node in the list to insert after
 */
static void LinkedList0_InsertAfter (LinkedList0 *list, LinkedList0Node *node, LinkedList0Node *target);

/**
 * Removes a node from the list.
 * 
 * @param list the list
 * @param node node to remove
 */
static void LinkedList0_Remove (LinkedList0 *list, LinkedList0Node *node);

/**
 * Returns the next node of a given node.
 * 
 * @param node reference node
 * @return next node, or NULL if none
 */
static LinkedList0Node * LinkedList0Node_Next (LinkedList0Node *node);

/**
 * Returns the previous node of a given node.
 * 
 * @param node reference node
 * @return previous node, or NULL if none
 */
static LinkedList0Node * LinkedList0Node_Prev (LinkedList0Node *node);

void LinkedList0_Init (LinkedList0 *list)
{
    list->first = NULL;
}

int LinkedList0_IsEmpty (LinkedList0 *list)
{
    return (!list->first);
}

LinkedList0Node * LinkedList0_GetFirst (LinkedList0 *list)
{
    return (list->first);
}

void LinkedList0_Prepend (LinkedList0 *list, LinkedList0Node *node)
{
    node->p = NULL;
    node->n = list->first;
    if (list->first) {
        list->first->p = node;
    }
    list->first = node;
}

void LinkedList0_InsertBefore (LinkedList0 *list, LinkedList0Node *node, LinkedList0Node *target)
{
    node->p = target->p;
    node->n = target;
    if (target->p) {
        target->p->n = node;
    } else {
        list->first = node;
    }
    target->p = node;
}

void LinkedList0_InsertAfter (LinkedList0 *list, LinkedList0Node *node, LinkedList0Node *target)
{
    node->p = target;
    node->n = target->n;
    if (target->n) {
        target->n->p = node;
    }
    target->n = node;
}

void LinkedList0_Remove (LinkedList0 *list, LinkedList0Node *node)
{
    // remove from list
    if (node->p) {
        node->p->n = node->n;
    } else {
        list->first = node->n;
    }
    if (node->n) {
        node->n->p = node->p;
    }
}

LinkedList0Node * LinkedList0Node_Next (LinkedList0Node *node)
{
    return node->n;
}

LinkedList0Node * LinkedList0Node_Prev (LinkedList0Node *node)
{
    return node->p;
}

#endif
