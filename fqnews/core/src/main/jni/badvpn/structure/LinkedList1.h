/**
 * @file LinkedList1.h
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
 * Simple doubly linked list.
 */

#ifndef BADVPN_STRUCTURE_LINKEDLIST1_H
#define BADVPN_STRUCTURE_LINKEDLIST1_H

#include <stddef.h>

#include <misc/debug.h>

/**
 * Linked list node.
 */
typedef struct LinkedList1Node_t
{
    struct LinkedList1Node_t *p;
    struct LinkedList1Node_t *n;
} LinkedList1Node;

/**
 * Simple doubly linked list.
 */
typedef struct
{
    LinkedList1Node *first;
    LinkedList1Node *last;
} LinkedList1;

/**
 * Initializes the linked list.
 * 
 * @param list list to initialize
 */
static void LinkedList1_Init (LinkedList1 *list);

/**
 * Determines if the list is empty.
 * 
 * @param list the list
 * @return 1 if empty, 0 if not
 */
static int LinkedList1_IsEmpty (LinkedList1 *list);

/**
 * Returns the first node of the list.
 * 
 * @param list the list
 * @return first node of the list, or NULL if the list is empty
 */
static LinkedList1Node * LinkedList1_GetFirst (LinkedList1 *list);

/**
 * Returns the last node of the list.
 * 
 * @param list the list
 * @return last node of the list, or NULL if the list is empty
 */
static LinkedList1Node * LinkedList1_GetLast (LinkedList1 *list);

/**
 * Inserts a node to the beginning of the list.
 * 
 * @param list the list
 * @param node uninitialized node to insert
 */
static void LinkedList1_Prepend (LinkedList1 *list, LinkedList1Node *node);

/**
 * Inserts a node to the end of the list.
 * 
 * @param list the list
 * @param node uninitialized node to insert
 */
static void LinkedList1_Append (LinkedList1 *list, LinkedList1Node *node);

/**
 * Inserts a node before a given node.
 * 
 * @param list the list
 * @param node uninitialized node to insert
 * @param target node in the list to insert before
 */
static void LinkedList1_InsertBefore (LinkedList1 *list, LinkedList1Node *node, LinkedList1Node *target);

/**
 * Inserts a node after a given node.
 * 
 * @param list the list
 * @param node uninitialized node to insert
 * @param target node in the list to insert after
 */
static void LinkedList1_InsertAfter (LinkedList1 *list, LinkedList1Node *node, LinkedList1Node *target);

/**
 * Removes a node from the list.
 * 
 * @param list the list
 * @param node node to remove
 */
static void LinkedList1_Remove (LinkedList1 *list, LinkedList1Node *node);

/**
 * Inserts the nodes in the list \a ins_list into this list, after the node \a target.
 * If \a target is NULL, the nodes are inserted to the beginning.
 * Note that because the first and last node in \a ins_list are modified
 * (unless the list is empty), \a ins_list is invalidated and must no longer
 * be used to access the inserted nodes.
 */
static void LinkedList1_InsertListAfter (LinkedList1 *list, LinkedList1 ins_list, LinkedList1Node *target);

/**
 * Returns the next node of a given node.
 * 
 * @param node reference node
 * @return next node, or NULL if none
 */
static LinkedList1Node * LinkedList1Node_Next (LinkedList1Node *node);

/**
 * Returns the previous node of a given node.
 * 
 * @param node reference node
 * @return previous node, or NULL if none
 */
static LinkedList1Node * LinkedList1Node_Prev (LinkedList1Node *node);

void LinkedList1_Init (LinkedList1 *list)
{
    list->first = NULL;
    list->last = NULL;
}

int LinkedList1_IsEmpty (LinkedList1 *list)
{
    return (!list->first);
}

LinkedList1Node * LinkedList1_GetFirst (LinkedList1 *list)
{
    return (list->first);
}

LinkedList1Node * LinkedList1_GetLast (LinkedList1 *list)
{
    return (list->last);
}

void LinkedList1_Prepend (LinkedList1 *list, LinkedList1Node *node)
{
    node->p = NULL;
    node->n = list->first;
    if (list->first) {
        list->first->p = node;
    } else {
        list->last = node;
    }
    list->first = node;
}

void LinkedList1_Append (LinkedList1 *list, LinkedList1Node *node)
{
    node->p = list->last;
    node->n = NULL;
    if (list->last) {
        list->last->n = node;
    } else {
        list->first = node;
    }
    list->last = node;
}

void LinkedList1_InsertBefore (LinkedList1 *list, LinkedList1Node *node, LinkedList1Node *target)
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

void LinkedList1_InsertAfter (LinkedList1 *list, LinkedList1Node *node, LinkedList1Node *target)
{
    node->p = target;
    node->n = target->n;
    if (target->n) {
        target->n->p = node;
    } else {
        list->last = node;
    }
    target->n = node;
}

void LinkedList1_Remove (LinkedList1 *list, LinkedList1Node *node)
{
    // remove from list
    if (node->p) {
        node->p->n = node->n;
    } else {
        list->first = node->n;
    }
    if (node->n) {
        node->n->p = node->p;
    } else {
        list->last = node->p;
    }
}

void LinkedList1_InsertListAfter (LinkedList1 *list, LinkedList1 ins_list, LinkedList1Node *target)
{
    if (!ins_list.first) {
        return;
    }
    
    LinkedList1Node *t_next = (target ? target->n : list->first);
    
    ins_list.first->p = target;
    ins_list.last->n = t_next;
    
    if (target) {
        target->n = ins_list.first;
    } else {
        list->first = ins_list.first;
    }
    
    if (t_next) {
        t_next->p = ins_list.last;
    } else {
        list->last = ins_list.last;
    }
}

LinkedList1Node * LinkedList1Node_Next (LinkedList1Node *node)
{
    return node->n;
}

LinkedList1Node * LinkedList1Node_Prev (LinkedList1Node *node)
{
    return node->p;
}

#endif
