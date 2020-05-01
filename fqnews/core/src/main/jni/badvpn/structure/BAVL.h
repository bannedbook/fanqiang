/**
 * @file BAVL.h
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
 * AVL tree.
 */

#ifndef BADVPN_STRUCTURE_BAVL_H
#define BADVPN_STRUCTURE_BAVL_H

//#define BAVL_DEBUG

#include <stdint.h>
#include <stddef.h>

#include <misc/debug.h>

/**
 * Handler function called by tree algorithms to compare two values.
 * For any two values, the comparator must always return the same result.
 * The <= relation defined by the comparator must be a total order.
 * Values are obtained like that:
 *   - The value of a node in the tree, or a node that is being inserted is:
 *     (uint8_t *)node + offset.
 *   - The value being looked up is the same as given to the lookup function.
 * 
 * @param user as in {@link BAVL_Init}
 * @param val1 first value
 * @param val2 second value
 * @return -1 if val1 < val2, 0 if val1 = val2, 1 if val1 > val2
 */
typedef int (*BAVL_comparator) (void *user, void *val1, void *val2);

struct BAVLNode;

/**
 * AVL tree.
 */
typedef struct {
    int offset;
    BAVL_comparator comparator;
    void *user;
    struct BAVLNode *root;
} BAVL;

/**
 * AVL tree node.
 */
typedef struct BAVLNode {
    struct BAVLNode *parent;
    struct BAVLNode *link[2];
    int8_t balance;
#ifdef BAVL_COUNT
    uint64_t count;
#endif
} BAVLNode;

/**
 * Initializes the tree.
 * 
 * @param o tree to initialize
 * @param offset offset of a value from its node
 * @param comparator value comparator handler function
 * @param user value to pass to comparator
 */
static void BAVL_Init (BAVL *o, int offset, BAVL_comparator comparator, void *user);

/**
 * Inserts a node into the tree.
 * Must not be called from comparator.
 * 
 * @param o the tree
 * @param node uninitialized node to insert. Must have a valid value (its value
 *             may be passed to the comparator during insertion).
 * @param ref if not NULL, will return (regardless if insertion succeeded):
 *              - the greatest node lesser than the inserted value, or (not in order)
 *              - the smallest node greater than the inserted value, or
 *              - NULL meaning there were no nodes in the tree.
 * @param 1 on success, 0 if an element with an equal value is already in the tree
 */
static int BAVL_Insert (BAVL *o, BAVLNode *node, BAVLNode **ref);

/**
 * Removes a node from the tree.
 * Must not be called from comparator.
 * 
 * @param o the tree
 * @param node node to remove
 */
static void BAVL_Remove (BAVL *o, BAVLNode *node);

/**
 * Checks if the tree is empty.
 * Must not be called from comparator.
 * 
 * @param o the tree
 * @return 1 if empty, 0 if not
 */
static int BAVL_IsEmpty (const BAVL *o);

/**
 * Looks for a value in the tree.
 * Must not be called from comparator.
 * 
 * @param o the tree
 * @param val value to look for
 * @return If a node is in the thee with an equal value, that node.
 *         Else if the tree is not empty:
 *           - the greatest node lesser than the given value, or (not in order)
 *           - the smallest node greater than the given value.
 *         NULL if the tree is empty.
 */
static BAVLNode * BAVL_Lookup (const BAVL *o, void *val);

/**
 * Looks for a value in the tree.
 * Must not be called from comparator.
 * 
 * @param o the tree
 * @param val value to look for
 * @return If a node is in the thee with an equal value, that node.
 *         Else NULL.
 */
static BAVLNode * BAVL_LookupExact (const BAVL *o, void *val);

/**
 * Returns the smallest node in the tree, or NULL if the tree is empty.
 * 
 * @param o the tree
 * @return smallest node or NULL
 */
static BAVLNode * BAVL_GetFirst (const BAVL *o);

/**
 * Returns the greatest node in the tree, or NULL if the tree is empty.
 * 
 * @param o the tree
 * @return greatest node or NULL
 */
static BAVLNode * BAVL_GetLast (const BAVL *o);

/**
 * Returns the node that follows the given node, or NULL if it's the
 * last node.
 * 
 * @param o the tree
 * @param n node
 * @return next node, or NULL
 */
static BAVLNode * BAVL_GetNext (const BAVL *o, BAVLNode *n);

/**
 * Returns the node that precedes the given node, or NULL if it's the
 * first node.
 * 
 * @param o the tree
 * @param n node
 * @return previous node, or NULL
 */
static BAVLNode * BAVL_GetPrev (const BAVL *o, BAVLNode *n);

#ifdef BAVL_COUNT
static uint64_t BAVL_Count (const BAVL *o);
static uint64_t BAVL_IndexOf (const BAVL *o, BAVLNode *n);
static BAVLNode * BAVL_GetAt (const BAVL *o, uint64_t index);
#endif

static void BAVL_Verify (BAVL *o);

#define BAVL_MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#define BAVL_OPTNEG(_a, _neg) ((_neg) ? -(_a) : (_a))

static void * _BAVL_node_value (const BAVL *o, BAVLNode *n)
{
    return ((uint8_t *)n + o->offset);
}

static int _BAVL_compare_values (const BAVL *o, void *v1, void *v2)
{
    int res = o->comparator(o->user, v1, v2);
    
    ASSERT(res == -1 || res == 0 || res == 1)
    
    return res;
}

static int _BAVL_compare_nodes (BAVL *o, BAVLNode *n1, BAVLNode *n2)
{
    return _BAVL_compare_values(o, _BAVL_node_value(o, n1), _BAVL_node_value(o, n2));
}

#ifdef BAVL_AUTO_VERIFY
#define BAVL_ASSERT(_h) BAVL_Verify((_h));
#else
#define BAVL_ASSERT(_h)
#endif

static int _BAVL_assert_recurser (BAVL *o, BAVLNode *n)
{
    ASSERT_FORCE(n->balance >= -1)
    ASSERT_FORCE(n->balance <= 1)
    
    int height_left = 0;
    int height_right = 0;
#ifdef BAVL_COUNT
    uint64_t count_left = 0;
    uint64_t count_right = 0;
#endif
    
    // check left subtree
    if (n->link[0]) {
        // check parent link
        ASSERT_FORCE(n->link[0]->parent == n)
        // check binary search tree
        ASSERT_FORCE(_BAVL_compare_nodes(o, n->link[0], n) == -1)
        // recursively calculate height
        height_left = _BAVL_assert_recurser(o, n->link[0]);
#ifdef BAVL_COUNT
        count_left = n->link[0]->count;
#endif
    }
    
    // check right subtree
    if (n->link[1]) {
        // check parent link
        ASSERT_FORCE(n->link[1]->parent == n)
        // check binary search tree
        ASSERT_FORCE(_BAVL_compare_nodes(o, n->link[1], n) == 1)
        // recursively calculate height
        height_right = _BAVL_assert_recurser(o, n->link[1]);
#ifdef BAVL_COUNT
        count_right = n->link[1]->count;
#endif
    }
    
    // check balance factor
    ASSERT_FORCE(n->balance == height_right - height_left)
    
#ifdef BAVL_COUNT
    // check count
    ASSERT_FORCE(n->count == 1 + count_left + count_right)
#endif
    
    return (BAVL_MAX(height_left, height_right) + 1);
}

#ifdef BAVL_COUNT
static void _BAVL_update_count_from_children (BAVLNode *n)
{
    n->count = 1 + (n->link[0] ? n->link[0]->count : 0) + (n->link[1] ? n->link[1]->count : 0);
}
#endif

static void _BAVL_rotate (BAVL *tree, BAVLNode *r, uint8_t dir)
{
    BAVLNode *nr = r->link[!dir];
    
    r->link[!dir] = nr->link[dir];
    if (r->link[!dir]) {
        r->link[!dir]->parent = r;
    }
    nr->link[dir] = r;
    nr->parent = r->parent;
    if (nr->parent) {
        nr->parent->link[r == r->parent->link[1]] = nr;
    } else {
        tree->root = nr;
    }
    r->parent = nr;
    
#ifdef BAVL_COUNT
    // update counts
    _BAVL_update_count_from_children(r); // first r!
    _BAVL_update_count_from_children(nr);
#endif
}

static BAVLNode * _BAVL_subtree_max (BAVLNode *n)
{
    ASSERT(n)
    while (n->link[1]) {
        n = n->link[1];
    }
    return n;
}

static void _BAVL_replace_subtree (BAVL *tree, BAVLNode *dest, BAVLNode *n)
{
    ASSERT(dest)
    
    if (dest->parent) {
        dest->parent->link[dest == dest->parent->link[1]] = n;
    } else {
        tree->root = n;
    }
    if (n) {
        n->parent = dest->parent;
    }
    
#ifdef BAVL_COUNT
    // update counts
    for (BAVLNode *c = dest->parent; c; c = c->parent) {
        ASSERT(c->count >= dest->count)
        c->count -= dest->count;
        if (n) {
            ASSERT(n->count <= UINT64_MAX - c->count)
            c->count += n->count;
        }
    }
#endif
}

static void _BAVL_swap_nodes (BAVL *tree, BAVLNode *n1, BAVLNode *n2)
{
    if (n2->parent == n1 || n1->parent == n2) {
        // when the nodes are directly connected we need special handling
        // make sure n1 is above n2
        if (n1->parent == n2) {
            BAVLNode *t = n1;
            n1 = n2;
            n2 = t;
        }
        
        uint8_t side = (n2 == n1->link[1]);
        BAVLNode *c = n1->link[!side];
        
        if (n1->link[0] = n2->link[0]) {
            n1->link[0]->parent = n1;
        }
        if (n1->link[1] = n2->link[1]) {
            n1->link[1]->parent = n1;
        }
        
        if (n2->parent = n1->parent) {
            n2->parent->link[n1 == n1->parent->link[1]] = n2;
        } else {
            tree->root = n2;
        }
        
        n2->link[side] = n1;
        n1->parent = n2;
        if (n2->link[!side] = c) {
            c->parent = n2;
        }
    } else {
        BAVLNode *temp;
        
        // swap parents
        temp = n1->parent;
        if (n1->parent = n2->parent) {
            n1->parent->link[n2 == n2->parent->link[1]] = n1;
        } else {
            tree->root = n1;
        }
        if (n2->parent = temp) {
            n2->parent->link[n1 == temp->link[1]] = n2;
        } else {
            tree->root = n2;
        }
        
        // swap left children
        temp = n1->link[0];
        if (n1->link[0] = n2->link[0]) {
            n1->link[0]->parent = n1;
        }
        if (n2->link[0] = temp) {
            n2->link[0]->parent = n2;
        }
        
        // swap right children
        temp = n1->link[1];
        if (n1->link[1] = n2->link[1]) {
            n1->link[1]->parent = n1;
        }
        if (n2->link[1] = temp) {
            n2->link[1]->parent = n2;
        }
    }
    
    // swap balance factors
    int8_t b = n1->balance;
    n1->balance = n2->balance;
    n2->balance = b;
    
#ifdef BAVL_COUNT
    // swap counts
    uint64_t c = n1->count;
    n1->count = n2->count;
    n2->count = c;
#endif
}

static void _BAVL_rebalance (BAVL *o, BAVLNode *node, uint8_t side, int8_t deltac)
{
    ASSERT(side == 0 || side == 1)
    ASSERT(deltac >= -1 && deltac <= 1)
    ASSERT(node->balance >= -1 && node->balance <= 1)
    
    // if no subtree changed its height, no more rebalancing is needed
    if (deltac == 0) {
        return;
    }
    
    // calculate how much our height changed
    int8_t delta = BAVL_MAX(deltac, BAVL_OPTNEG(node->balance, side)) - BAVL_MAX(0, BAVL_OPTNEG(node->balance, side));
    ASSERT(delta >= -1 && delta <= 1)
    
    // update our balance factor
    node->balance -= BAVL_OPTNEG(deltac, side);
    
    BAVLNode *child;
    BAVLNode *gchild;
    
    // perform transformations if the balance factor is wrong
    if (node->balance == 2 || node->balance == -2) {
        uint8_t bside;
        int8_t bsidef;
        if (node->balance == 2) {
            bside = 1;
            bsidef = 1;
        } else {
            bside = 0;
            bsidef = -1;
        }
        
        ASSERT(node->link[bside])
        child = node->link[bside];
        switch (child->balance * bsidef) {
            case 1:
                _BAVL_rotate(o, node, !bside);
                node->balance = 0;
                child->balance = 0;
                node = child;
                delta -= 1;
                break;
            case 0:
                _BAVL_rotate(o, node, !bside);
                node->balance = 1 * bsidef;
                child->balance = -1 * bsidef;
                node = child;
                break;
            case -1:
                ASSERT(child->link[!bside])
                gchild = child->link[!bside];
                _BAVL_rotate(o, child, bside);
                _BAVL_rotate(o, node, !bside);
                node->balance = -BAVL_MAX(0, gchild->balance * bsidef) * bsidef;
                child->balance = BAVL_MAX(0, -gchild->balance * bsidef) * bsidef;
                gchild->balance = 0;
                node = gchild;
                delta -= 1;
                break;
            default:
                ASSERT(0);
        }
    }
    
    ASSERT(delta >= -1 && delta <= 1)
    // Transformations above preserve this. Proof:
    //     - if a child subtree gained 1 height and rebalancing was needed,
    //       it was the heavier subtree. Then delta was was originally 1, because
    //       the heaviest subtree gained one height. If the transformation reduces
    //       delta by one, it becomes 0.
    //     - if a child subtree lost 1 height and rebalancing was needed, it
    //       was the lighter subtree. Then delta was originally 0, because
    //       the height of the heaviest subtree was unchanged. If the transformation
    //       reduces delta by one, it becomes -1.
    
    if (node->parent) {
        _BAVL_rebalance(o, node->parent, node == node->parent->link[1], delta);
    }
}

void BAVL_Init (BAVL *o, int offset, BAVL_comparator comparator, void *user)
{
    o->offset = offset;
    o->comparator = comparator;
    o->user = user;
    o->root = NULL;
    
    BAVL_ASSERT(o)
}

int BAVL_Insert (BAVL *o, BAVLNode *node, BAVLNode **ref)
{
    // insert to root?
    if (!o->root) {
        o->root = node;
        node->parent = NULL;
        node->link[0] = NULL;
        node->link[1] = NULL;
        node->balance = 0;
#ifdef BAVL_COUNT
        node->count = 1;
#endif
        
        BAVL_ASSERT(o)
        
        if (ref) {
            *ref = NULL;
        }
        return 1;
    }
    
    // find node to insert to
    BAVLNode *c = o->root;
    int side;
    while (1) {
        // compare
        int comp = _BAVL_compare_nodes(o, node, c);
        
        // have we found a node that compares equal?
        if (comp == 0) {
            if (ref) {
                *ref = c;
            }
            return 0;
        }
        
        side = (comp == 1);
        
        // have we reached a leaf?
        if (!c->link[side]) {
            break;
        }
        
        c = c->link[side];
    }
    
    // insert
    c->link[side] = node;
    node->parent = c;
    node->link[0] = NULL;
    node->link[1] = NULL;
    node->balance = 0;
#ifdef BAVL_COUNT
    node->count = 1;
#endif
    
#ifdef BAVL_COUNT
    // update counts
    for (BAVLNode *p = c; p; p = p->parent) {
        ASSERT(p->count < UINT64_MAX)
        p->count++;
    }
#endif
    
    // rebalance
    _BAVL_rebalance(o, c, side, 1);
    
    BAVL_ASSERT(o)
    
    if (ref) {
        *ref = c;
    }
    return 1;
}

void BAVL_Remove (BAVL *o, BAVLNode *node)
{
    // if we have both subtrees, swap the node and the largest node
    // in the left subtree, so we have at most one subtree
    if (node->link[0] && node->link[1]) {
        // find the largest node in the left subtree
        BAVLNode *max = _BAVL_subtree_max(node->link[0]);
        // swap the nodes
        _BAVL_swap_nodes(o, node, max);
    }
    
    // have at most one child now
    ASSERT(!(node->link[0] && node->link[1]))
    
    BAVLNode *parent = node->parent;
    BAVLNode *child = (node->link[0] ? node->link[0] : node->link[1]);
    
    if (parent) {
        // remember on which side node is
        int side = (node == parent->link[1]);
        // replace node with child
        _BAVL_replace_subtree(o, node, child);
        // rebalance
        _BAVL_rebalance(o, parent, side, -1);
    } else {
        // replace node with child
        _BAVL_replace_subtree(o, node, child);
    }
    
    BAVL_ASSERT(o)
}

int BAVL_IsEmpty (const BAVL *o)
{
    return (!o->root);
}

BAVLNode * BAVL_Lookup (const BAVL *o, void *val)
{
    if (!o->root) {
        return NULL;
    }
    
    BAVLNode *c = o->root;
    while (1) {
        // compare
        int comp = _BAVL_compare_values(o, val, _BAVL_node_value(o, c));
        
        // have we found a node that compares equal?
        if (comp == 0) {
            return c;
        }
        
        int side = (comp == 1);
        
        // have we reached a leaf?
        if (!c->link[side]) {
            return c;
        }
        
        c = c->link[side];
    }
}

BAVLNode * BAVL_LookupExact (const BAVL *o, void *val)
{
    if (!o->root) {
        return NULL;
    }
    
    BAVLNode *c = o->root;
    while (1) {
        // compare
        int comp = _BAVL_compare_values(o, val, _BAVL_node_value(o, c));
        
        // have we found a node that compares equal?
        if (comp == 0) {
            return c;
        }
        
        int side = (comp == 1);
        
        // have we reached a leaf?
        if (!c->link[side]) {
            return NULL;
        }
        
        c = c->link[side];
    }
}

BAVLNode * BAVL_GetFirst (const BAVL *o)
{
    if (!o->root) {
        return NULL;
    }
    
    BAVLNode *n = o->root;
    while (n->link[0]) {
        n = n->link[0];
    }
    
    return n;
}

BAVLNode * BAVL_GetLast (const BAVL *o)
{
    if (!o->root) {
        return NULL;
    }
    
    BAVLNode *n = o->root;
    while (n->link[1]) {
        n = n->link[1];
    }
    
    return n;
}

BAVLNode * BAVL_GetNext (const BAVL *o, BAVLNode *n)
{
    if (n->link[1]) {
        n = n->link[1];
        while (n->link[0]) {
            n = n->link[0];
        }
    } else {
        while (n->parent && n == n->parent->link[1]) {
            n = n->parent;
        }
        n = n->parent;
    }
    
    return n;
}

BAVLNode * BAVL_GetPrev (const BAVL *o, BAVLNode *n)
{
    if (n->link[0]) {
        n = n->link[0];
        while (n->link[1]) {
            n = n->link[1];
        }
    } else {
        while (n->parent && n == n->parent->link[0]) {
            n = n->parent;
        }
        n = n->parent;
    }
    
    return n;
}

#ifdef BAVL_COUNT

static uint64_t BAVL_Count (const BAVL *o)
{
    return (o->root ? o->root->count : 0);
}

static uint64_t BAVL_IndexOf (const BAVL *o, BAVLNode *n)
{
    uint64_t index = (n->link[0] ? n->link[0]->count : 0);
    
    for (BAVLNode *c = n; c->parent; c = c->parent) {
        if (c == c->parent->link[1]) {
            ASSERT(c->parent->count > c->count)
            ASSERT(c->parent->count - c->count <= UINT64_MAX - index)
            index += c->parent->count - c->count;
        }
    }
    
    return index;
}

static BAVLNode * BAVL_GetAt (const BAVL *o, uint64_t index)
{
    if (index >= BAVL_Count(o)) {
        return NULL;
    }
    
    BAVLNode *c = o->root;
    
    while (1) {
        ASSERT(c)
        ASSERT(index < c->count)
        
        uint64_t left_count = (c->link[0] ? c->link[0]->count : 0);
        
        if (index == left_count) {
            return c;
        }
        
        if (index < left_count) {
            c = c->link[0];
        } else {
            c = c->link[1];
            index -= left_count + 1;
        }
    }
}

#endif

static void BAVL_Verify (BAVL *o)
{
    if (o->root) {
        ASSERT(!o->root->parent)
        _BAVL_assert_recurser(o, o->root);
    }
}

#endif
