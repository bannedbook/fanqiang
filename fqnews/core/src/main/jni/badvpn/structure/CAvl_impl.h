/**
 * @file CAvl_impl.h
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

#include "CAvl_header.h"

static CAvlLink CAvl_nulllink (void)
{
    return CAVL_PARAM_VALUE_NULL;
}

static CAvlRef CAvl_nullref (void)
{
    CAvlRef n;
    n.link = CAVL_PARAM_VALUE_NULL;
    n.ptr = NULL;
    return n;
}

#if !CAVL_PARAM_FEATURE_KEYS_ARE_INDICES

static int CAvl_compare_entries (CAvlArg arg, CAvlRef node1, CAvlRef node2)
{
    int res = CAVL_PARAM_FUN_COMPARE_ENTRIES(arg, node1, node2);
    ASSERT(res >= -1)
    ASSERT(res <= 1)
    
    return res;
}

#if !CAVL_PARAM_FEATURE_NOKEYS

static int CAvl_compare_key_entry (CAvlArg arg, CAvlKey key1, CAvlRef node2)
{
    int res = CAVL_PARAM_FUN_COMPARE_KEY_ENTRY(arg, key1, node2);
    ASSERT(res >= -1)
    ASSERT(res <= 1)
    
    return res;
}

#endif

#endif

#if CAVL_PARAM_FEATURE_ASSOC

static CAvlAssoc CAvl_compute_node_assoc (CAvlArg arg, CAvlRef node)
{
    CAvlAssoc sum = CAVL_PARAM_FUN_ASSOC_VALUE(arg, node);
    if (CAvl_link(node)[0] != CAvl_nulllink()) {
        sum = CAVL_PARAM_FUN_ASSOC_OPER(arg, CAvl_assoc(CAvlDeref(arg, CAvl_link(node)[0])), sum);
    }
    if (CAvl_link(node)[1] != CAvl_nulllink()) {
        sum = CAVL_PARAM_FUN_ASSOC_OPER(arg, sum, CAvl_assoc(CAvlDeref(arg, CAvl_link(node)[1])));
    }
    return sum;
}

#endif

static int CAvl_check_parent (CAvlRef p, CAvlRef c)
{
    return (p.link == CAvl_parent(c)) && (p.link == CAvl_nulllink() || c.link == CAvl_link(p)[0] || c.link == CAvl_link(p)[1]);
}

static int CAvl_verify_recurser (CAvlArg arg, CAvlRef n)
{
    ASSERT_FORCE(CAvl_balance(n) >= -1)
    ASSERT_FORCE(CAvl_balance(n) <= 1)
    
    int height_left = 0;
    int height_right = 0;
#if CAVL_PARAM_FEATURE_COUNTS
    CAvlCount count_left = 0;
    CAvlCount count_right = 0;
#endif
    
    // check left subtree
    if (CAvl_link(n)[0] != CAvl_nulllink()) {
        // check parent link
        ASSERT_FORCE(CAvl_parent(CAvlDeref(arg, CAvl_link(n)[0])) == n.link)
        // check binary search tree
#if !CAVL_PARAM_FEATURE_KEYS_ARE_INDICES
        ASSERT_FORCE(CAvl_compare_entries(arg, CAvlDeref(arg, CAvl_link(n)[0]), n) == -1)
#endif
        // recursively calculate height
        height_left = CAvl_verify_recurser(arg, CAvlDeref(arg, CAvl_link(n)[0]));
#if CAVL_PARAM_FEATURE_COUNTS
        count_left = CAvl_count(CAvlDeref(arg, CAvl_link(n)[0]));
#endif
    }
    
    // check right subtree
    if (CAvl_link(n)[1] != CAvl_nulllink()) {
        // check parent link
        ASSERT_FORCE(CAvl_parent(CAvlDeref(arg, CAvl_link(n)[1])) == n.link)
        // check binary search tree
#if !CAVL_PARAM_FEATURE_KEYS_ARE_INDICES
        ASSERT_FORCE(CAvl_compare_entries(arg, CAvlDeref(arg, CAvl_link(n)[1]), n) == 1)
#endif
        // recursively calculate height
        height_right = CAvl_verify_recurser(arg, CAvlDeref(arg, CAvl_link(n)[1]));
#if CAVL_PARAM_FEATURE_COUNTS
        count_right = CAvl_count(CAvlDeref(arg, CAvl_link(n)[1]));
#endif
    }
    
    // check balance factor
    ASSERT_FORCE(CAvl_balance(n) == height_right - height_left)
    
#if CAVL_PARAM_FEATURE_COUNTS
    // check count
    ASSERT_FORCE(CAvl_count(n) == 1 + count_left + count_right)
#endif
    
#if CAVL_PARAM_FEATURE_ASSOC
    // check assoc
    ASSERT_FORCE(CAvl_assoc(n) == CAvl_compute_node_assoc(arg, n))
#endif
    
    return CAvl_MAX(height_left, height_right) + 1;
}

static void CAvl_assert_tree (CAvl *o, CAvlArg arg)
{
#ifdef CAVL_AUTO_VERIFY
    CAvl_Verify(o, arg);
#endif
}

#if CAVL_PARAM_FEATURE_COUNTS
static void CAvl_update_count_from_children (CAvlArg arg, CAvlRef n)
{
    CAvlCount left_count = CAvl_link(n)[0] != CAvl_nulllink() ? CAvl_count(CAvlDeref(arg, CAvl_link(n)[0])) : 0;
    CAvlCount right_count = CAvl_link(n)[1] != CAvl_nulllink() ? CAvl_count(CAvlDeref(arg, CAvl_link(n)[1])) : 0;
    CAvl_count(n) = 1 + left_count + right_count;
}
#endif

static void CAvl_rotate (CAvl *o, CAvlArg arg, CAvlRef r, uint8_t dir, CAvlRef r_parent)
{
    ASSERT(CAvl_check_parent(r_parent, r))
    CAvlRef nr = CAvlDeref(arg, CAvl_link(r)[!dir]);
    
    CAvl_link(r)[!dir] = CAvl_link(nr)[dir];
    if (CAvl_link(r)[!dir] != CAvl_nulllink()) {
        CAvl_parent(CAvlDeref(arg, CAvl_link(r)[!dir])) = r.link;
    }
    CAvl_link(nr)[dir] = r.link;
    CAvl_parent(nr) = r_parent.link;
    if (r_parent.link != CAvl_nulllink()) {
        CAvl_link(r_parent)[r.link == CAvl_link(r_parent)[1]] = nr.link;
    } else {
        o->root = nr.link;
    }
    CAvl_parent(r) = nr.link;
    
#if CAVL_PARAM_FEATURE_COUNTS
    CAvl_update_count_from_children(arg, r);
    CAvl_update_count_from_children(arg, nr);
#endif
    
#if CAVL_PARAM_FEATURE_ASSOC
    CAvl_assoc(r) = CAvl_compute_node_assoc(arg, r);
    CAvl_assoc(nr) = CAvl_compute_node_assoc(arg, nr);
#endif
}

static CAvlRef CAvl_subtree_min (CAvlArg arg, CAvlRef n)
{
    ASSERT(n.link != CAvl_nulllink())
    
    while (CAvl_link(n)[0] != CAvl_nulllink()) {
        n = CAvlDeref(arg, CAvl_link(n)[0]);
    }
    
    return n;
}

static CAvlRef CAvl_subtree_max (CAvlArg arg, CAvlRef n)
{
    ASSERT(n.link != CAvl_nulllink())
    
    while (CAvl_link(n)[1] != CAvl_nulllink()) {
        n = CAvlDeref(arg, CAvl_link(n)[1]);
    }
    
    return n;
}

static void CAvl_replace_subtree_fix_assoc (CAvl *o, CAvlArg arg, CAvlRef dest, CAvlRef n, CAvlRef dest_parent)
{
    ASSERT(dest.link != CAvl_nulllink())
    ASSERT(CAvl_check_parent(dest_parent, dest))
    
    if (dest_parent.link != CAvl_nulllink()) {
        CAvl_link(dest_parent)[dest.link == CAvl_link(dest_parent)[1]] = n.link;
    } else {
        o->root = n.link;
    }
    if (n.link != CAvl_nulllink()) {
        CAvl_parent(n) = CAvl_parent(dest);
    }
    
#if CAVL_PARAM_FEATURE_COUNTS || CAVL_PARAM_FEATURE_ASSOC
    for (CAvlRef c = dest_parent; c.link != CAvl_nulllink(); c = CAvlDeref(arg, CAvl_parent(c))) {
#if CAVL_PARAM_FEATURE_COUNTS
        ASSERT(CAvl_count(c) >= CAvl_count(dest))
        CAvl_count(c) -= CAvl_count(dest);
        if (n.link != CAvl_nulllink()) {
            ASSERT(CAvl_count(n) <= CAVL_PARAM_VALUE_COUNT_MAX - CAvl_count(c))
            CAvl_count(c) += CAvl_count(n);
        }
#endif
#if CAVL_PARAM_FEATURE_ASSOC
        CAvl_assoc(c) = CAvl_compute_node_assoc(arg, c);
#endif
    }
#endif
}

static void CAvl_swap_for_remove (CAvl *o, CAvlArg arg, CAvlRef node, CAvlRef enode, CAvlRef node_parent, CAvlRef enode_parent)
{
    ASSERT(CAvl_check_parent(node_parent, node))
    ASSERT(CAvl_check_parent(enode_parent, enode))
    
    if (enode_parent.link == node.link) {
        // when the nodes are directly connected we need special handling
        
        uint8_t side = (enode.link == CAvl_link(node)[1]);
        CAvlRef c = CAvlDeref(arg, CAvl_link(node)[!side]);
        
        if ((CAvl_link(node)[0] = CAvl_link(enode)[0]) != CAvl_nulllink()) {
            CAvl_parent(CAvlDeref(arg, CAvl_link(node)[0])) = node.link;
        }
        if ((CAvl_link(node)[1] = CAvl_link(enode)[1]) != CAvl_nulllink()) {
            CAvl_parent(CAvlDeref(arg, CAvl_link(node)[1])) = node.link;
        }
        
        CAvl_parent(enode) = CAvl_parent(node);
        if (node_parent.link != CAvl_nulllink()) {
            CAvl_link(node_parent)[node.link == CAvl_link(node_parent)[1]] = enode.link;
        } else {
            o->root = enode.link;
        }
        
        CAvl_link(enode)[side] = node.link;
        CAvl_parent(node) = enode.link;
        if ((CAvl_link(enode)[!side] = c.link) != CAvl_nulllink()) {
            CAvl_parent(c) = enode.link;
        }
    } else {
        CAvlRef temp;
        
        // swap parents
        temp = node_parent;
        CAvl_parent(node) = CAvl_parent(enode);
        if (enode_parent.link != CAvl_nulllink()) {
            CAvl_link(enode_parent)[enode.link == CAvl_link(enode_parent)[1]] = node.link;
        } else {
            o->root = node.link;
        }
        CAvl_parent(enode) = temp.link;
        if (temp.link != CAvl_nulllink()) {
            CAvl_link(temp)[node.link == CAvl_link(temp)[1]] = enode.link;
        } else {
            o->root = enode.link;
        }
        
        // swap left children
        temp = CAvlDeref(arg, CAvl_link(node)[0]);
        if ((CAvl_link(node)[0] = CAvl_link(enode)[0]) != CAvl_nulllink()) {
            CAvl_parent(CAvlDeref(arg, CAvl_link(node)[0])) = node.link;
        }
        if ((CAvl_link(enode)[0] = temp.link) != CAvl_nulllink()) {
            CAvl_parent(CAvlDeref(arg, CAvl_link(enode)[0])) = enode.link;
        }
        
        // swap right children
        temp = CAvlDeref(arg, CAvl_link(node)[1]);
        if ((CAvl_link(node)[1] = CAvl_link(enode)[1]) != CAvl_nulllink()) {
            CAvl_parent(CAvlDeref(arg, CAvl_link(node)[1])) = node.link;
        }
        if ((CAvl_link(enode)[1] = temp.link) != CAvl_nulllink()) {
            CAvl_parent(CAvlDeref(arg, CAvl_link(enode)[1])) = enode.link;
        }
    }
    
    // swap balance factors
    int8_t b = CAvl_balance(node);
    CAvl_balance(node) = CAvl_balance(enode);
    CAvl_balance(enode) = b;
    
#if CAVL_PARAM_FEATURE_COUNTS
    // swap counts
    CAvlCount c = CAvl_count(node);
    CAvl_count(node) = CAvl_count(enode);
    CAvl_count(enode) = c;
#endif
    
    // not fixing assoc values here because CAvl_replace_subtree_fix_assoc() will do it
}

static void CAvl_rebalance (CAvl *o, CAvlArg arg, CAvlRef node, uint8_t side, int8_t deltac)
{
    ASSERT(side == 0 || side == 1)
    ASSERT(deltac >= -1 && deltac <= 1)
    ASSERT(CAvl_balance(node) >= -1 && CAvl_balance(node) <= 1)
    
    // if no subtree changed its height, no more rebalancing is needed
    if (deltac == 0) {
        return;
    }
    
    // calculate how much our height changed
    int8_t delta = CAvl_MAX(deltac, CAvl_OPTNEG(CAvl_balance(node), side)) - CAvl_MAX(0, CAvl_OPTNEG(CAvl_balance(node), side));
    ASSERT(delta >= -1 && delta <= 1)
    
    // update our balance factor
    CAvl_balance(node) -= CAvl_OPTNEG(deltac, side);
    
    CAvlRef child;
    CAvlRef gchild;
    
    // perform transformations if the balance factor is wrong
    if (CAvl_balance(node) == 2 || CAvl_balance(node) == -2) {
        uint8_t bside;
        int8_t bsidef;
        if (CAvl_balance(node) == 2) {
            bside = 1;
            bsidef = 1;
        } else {
            bside = 0;
            bsidef = -1;
        }
        
        ASSERT(CAvl_link(node)[bside] != CAvl_nulllink())
        child = CAvlDeref(arg, CAvl_link(node)[bside]);
        
        switch (CAvl_balance(child) * bsidef) {
            case 1:
                CAvl_rotate(o, arg, node, !bside, CAvlDeref(arg, CAvl_parent(node)));
                CAvl_balance(node) = 0;
                CAvl_balance(child) = 0;
                node = child;
                delta -= 1;
                break;
            case 0:
                CAvl_rotate(o, arg, node, !bside, CAvlDeref(arg, CAvl_parent(node)));
                CAvl_balance(node) = 1 * bsidef;
                CAvl_balance(child) = -1 * bsidef;
                node = child;
                break;
            case -1:
                ASSERT(CAvl_link(child)[!bside] != CAvl_nulllink())
                gchild = CAvlDeref(arg, CAvl_link(child)[!bside]);
                CAvl_rotate(o, arg, child, bside, node);
                CAvl_rotate(o, arg, node, !bside, CAvlDeref(arg, CAvl_parent(node)));
                CAvl_balance(node) = -CAvl_MAX(0, CAvl_balance(gchild) * bsidef) * bsidef;
                CAvl_balance(child) = CAvl_MAX(0, -CAvl_balance(gchild) * bsidef) * bsidef;
                CAvl_balance(gchild) = 0;
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
    
    if (CAvl_parent(node) != CAvl_nulllink()) {
        CAvlRef node_parent = CAvlDeref(arg, CAvl_parent(node));
        CAvl_rebalance(o, arg, node_parent, node.link == CAvl_link(node_parent)[1], delta);
    }
}

#if CAVL_PARAM_FEATURE_KEYS_ARE_INDICES
static CAvlCount CAvl_child_count (CAvlArg arg, CAvlRef n, int dir)
{
    return (CAvl_link(n)[dir] != CAvl_nulllink() ? CAvl_count(CAvlDeref(arg, CAvl_link(n)[dir])) : 0);
}
#endif

static int CAvlIsNullRef (CAvlRef node)
{
    return node.link == CAvl_nulllink();
}

static int CAvlIsValidRef (CAvlRef node)
{
    return node.link != CAvl_nulllink();
}

static CAvlRef CAvlDeref (CAvlArg arg, CAvlLink link)
{
    if (link == CAvl_nulllink()) {
        return CAvl_nullref();
    }
    
    CAvlRef n;
    n.ptr = CAVL_PARAM_FUN_DEREF(arg, link);
    n.link = link;
    
    ASSERT(n.ptr)
    
    return n;
}

static void CAvl_Init (CAvl *o)
{
    o->root = CAvl_nulllink();
}

#if !CAVL_PARAM_FEATURE_KEYS_ARE_INDICES

static int CAvl_Insert (CAvl *o, CAvlArg arg, CAvlRef node, CAvlRef *out_ref)
{
    ASSERT(node.link != CAvl_nulllink())
#if CAVL_PARAM_FEATURE_COUNTS
    ASSERT(CAvl_Count(o, arg) < CAVL_PARAM_VALUE_COUNT_MAX)
#endif
    
    // insert to root?
    if (o->root == CAvl_nulllink()) {
        o->root = node.link;
        CAvl_parent(node) = CAvl_nulllink();
        CAvl_link(node)[0] = CAvl_nulllink();
        CAvl_link(node)[1] = CAvl_nulllink();
        CAvl_balance(node) = 0;
#if CAVL_PARAM_FEATURE_COUNTS
        CAvl_count(node) = 1;
#endif
#if CAVL_PARAM_FEATURE_ASSOC
        CAvl_assoc(node) = CAVL_PARAM_FUN_ASSOC_VALUE(arg, node);
#endif
        
        CAvl_assert_tree(o, arg);
        
        if (out_ref) {
            *out_ref = CAvl_nullref();
        }
        return 1;
    }
    
    CAvlRef c = CAvlDeref(arg, o->root);
    int side;
    while (1) {
        int comp = CAvl_compare_entries(arg, node, c);
        
        if (comp == 0) {
            if (out_ref) {
                *out_ref = c;
            }
            return 0;
        }
        
        side = (comp == 1);
        
        if (CAvl_link(c)[side] == CAvl_nulllink()) {
            break;
        }
        
        c = CAvlDeref(arg, CAvl_link(c)[side]);
    }
    
    CAvl_link(c)[side] = node.link;
    CAvl_parent(node) = c.link;
    CAvl_link(node)[0] = CAvl_nulllink();
    CAvl_link(node)[1] = CAvl_nulllink();
    CAvl_balance(node) = 0;
#if CAVL_PARAM_FEATURE_COUNTS
    CAvl_count(node) = 1;
#endif
#if CAVL_PARAM_FEATURE_ASSOC
    CAvl_assoc(node) = CAVL_PARAM_FUN_ASSOC_VALUE(arg, node);
#endif
    
#if CAVL_PARAM_FEATURE_COUNTS || CAVL_PARAM_FEATURE_ASSOC
    for (CAvlRef p = c; p.link != CAvl_nulllink(); p = CAvlDeref(arg, CAvl_parent(p))) {
#if CAVL_PARAM_FEATURE_COUNTS
        CAvl_count(p)++;
#endif
#if CAVL_PARAM_FEATURE_ASSOC
        CAvl_assoc(p) = CAvl_compute_node_assoc(arg, p);
#endif
    }
#endif
    
    CAvl_rebalance(o, arg, c, side, 1);
    
    CAvl_assert_tree(o, arg);
    
    if (out_ref) {
        *out_ref = c;
    }
    return 1;
}

#else

static void CAvl_InsertAt (CAvl *o, CAvlArg arg, CAvlRef node, CAvlCount index)
{
    ASSERT(node.link != CAvl_nulllink())
    ASSERT(index <= CAvl_Count(o, arg))
    ASSERT(CAvl_Count(o, arg) < CAVL_PARAM_VALUE_COUNT_MAX)
    
    // insert to root?
    if (o->root == CAvl_nulllink()) {
        o->root = node.link;
        CAvl_parent(node) = CAvl_nulllink();
        CAvl_link(node)[0] = CAvl_nulllink();
        CAvl_link(node)[1] = CAvl_nulllink();
        CAvl_balance(node) = 0;
        CAvl_count(node) = 1;
#if CAVL_PARAM_FEATURE_ASSOC
        CAvl_assoc(node) = CAVL_PARAM_FUN_ASSOC_VALUE(arg, node);
#endif
        
        CAvl_assert_tree(o, arg);
        return;
    }
    
    CAvlRef c = CAvlDeref(arg, o->root);
    CAvlCount c_idx = CAvl_child_count(arg, c, 0);
    int side;
    while (1) {
        side = (index > c_idx);
        
        if (CAvl_link(c)[side] == CAvl_nulllink()) {
            break;
        }
        
        c = CAvlDeref(arg, CAvl_link(c)[side]);
        
        if (side == 0) {
            c_idx -= 1 + CAvl_child_count(arg, c, 1);
        } else {
            c_idx += 1 + CAvl_child_count(arg, c, 0);
        }
    }
    
    CAvl_link(c)[side] = node.link;
    CAvl_parent(node) = c.link;
    CAvl_link(node)[0] = CAvl_nulllink();
    CAvl_link(node)[1] = CAvl_nulllink();
    CAvl_balance(node) = 0;
    CAvl_count(node) = 1;
#if CAVL_PARAM_FEATURE_ASSOC
    CAvl_assoc(node) = CAVL_PARAM_FUN_ASSOC_VALUE(arg, node);
#endif
    
    for (CAvlRef p = c; p.link != CAvl_nulllink(); p = CAvlDeref(arg, CAvl_parent(p))) {
        CAvl_count(p)++;
#if CAVL_PARAM_FEATURE_ASSOC
        CAvl_assoc(p) = CAvl_compute_node_assoc(arg, p);
#endif
    }
    
    CAvl_rebalance(o, arg, c, side, 1);
    
    CAvl_assert_tree(o, arg);
    return;
}

#endif

static void CAvl_Remove (CAvl *o, CAvlArg arg, CAvlRef node)
{
    ASSERT(node.link != CAvl_nulllink())
    ASSERT(o->root != CAvl_nulllink())
    
    if (CAvl_link(node)[0] != CAvl_nulllink() && CAvl_link(node)[1] != CAvl_nulllink()) {
        CAvlRef max = CAvl_subtree_max(arg, CAvlDeref(arg, CAvl_link(node)[0]));
        CAvl_swap_for_remove(o, arg, node, max, CAvlDeref(arg, CAvl_parent(node)), CAvlDeref(arg, CAvl_parent(max)));
    }
    
    ASSERT(CAvl_link(node)[0] == CAvl_nulllink() || CAvl_link(node)[1] == CAvl_nulllink())
    
    CAvlRef paren = CAvlDeref(arg, CAvl_parent(node));
    CAvlRef child = (CAvl_link(node)[0] != CAvl_nulllink() ? CAvlDeref(arg, CAvl_link(node)[0]) : CAvlDeref(arg, CAvl_link(node)[1]));
    
    if (paren.link != CAvl_nulllink()) {
        int side = (node.link == CAvl_link(paren)[1]);
        CAvl_replace_subtree_fix_assoc(o, arg, node, child, paren);
        CAvl_rebalance(o, arg, paren, side, -1);
    } else {
        CAvl_replace_subtree_fix_assoc(o, arg, node, child, paren);
    }
    
    CAvl_assert_tree(o, arg);
}

#if !CAVL_PARAM_FEATURE_KEYS_ARE_INDICES && !CAVL_PARAM_FEATURE_NOKEYS

static CAvlRef CAvl_Lookup (const CAvl *o, CAvlArg arg, CAvlKey key)
{
    if (o->root == CAvl_nulllink()) {
        return CAvl_nullref();
    }
    
    CAvlRef c = CAvlDeref(arg, o->root);
    while (1) {
        // compare
        int comp = CAvl_compare_key_entry(arg, key, c);
        
        // have we found a node that compares equal?
        if (comp == 0) {
            return c;
        }
        
        int side = (comp == 1);
        
        // have we reached a leaf?
        if (CAvl_link(c)[side] == CAvl_nulllink()) {
            return c;
        }
        
        c = CAvlDeref(arg, CAvl_link(c)[side]);
    }
}

static CAvlRef CAvl_LookupExact (const CAvl *o, CAvlArg arg, CAvlKey key)
{
    if (o->root == CAvl_nulllink()) {
        return CAvl_nullref();
    }
    
    CAvlRef c = CAvlDeref(arg, o->root);
    while (1) {
        // compare
        int comp = CAvl_compare_key_entry(arg, key, c);
        
        // have we found a node that compares equal?
        if (comp == 0) {
            return c;
        }
        
        int side = (comp == 1);
        
        // have we reached a leaf?
        if (CAvl_link(c)[side] == CAvl_nulllink()) {
            return CAvl_nullref();
        }
        
        c = CAvlDeref(arg, CAvl_link(c)[side]);
    }
}

static CAvlRef CAvl_GetFirstGreater (const CAvl *o, CAvlArg arg, CAvlKey key)
{
    CAvlRef c = CAvl_Lookup(o, arg, key);
    if (CAvlIsNullRef(c)) {
        return CAvl_nullref();
    }
    
    if (CAvl_compare_key_entry(arg, key, c) >= 0) {
        c = CAvl_GetNext(o, arg, c);
        if (CAvlIsNullRef(c)) {
            return CAvl_nullref();
        }
    }
    
    ASSERT(CAvl_compare_key_entry(arg, key, c) < 0);
    
    return c;
}

static CAvlRef CAvl_GetLastLesser (const CAvl *o, CAvlArg arg, CAvlKey key)
{
    CAvlRef c = CAvl_Lookup(o, arg, key);
    if (CAvlIsNullRef(c)) {
        return CAvl_nullref();
    }
    
    if (CAvl_compare_key_entry(arg, key, c) <= 0) {
        c = CAvl_GetPrev(o, arg, c);
        if (CAvlIsNullRef(c)) {
            return CAvl_nullref();
        }
    }
    
    ASSERT(CAvl_compare_key_entry(arg, key, c) > 0);
    
    return c;
}

static CAvlRef CAvl_GetFirstGreaterEqual (const CAvl *o, CAvlArg arg, CAvlKey key)
{
    CAvlRef c = CAvl_Lookup(o, arg, key);
    if (CAvlIsNullRef(c)) {
        return CAvl_nullref();
    }
    
    if (CAvl_compare_key_entry(arg, key, c) > 0) {
        c = CAvl_GetNext(o, arg, c);
        if (CAvlIsNullRef(c)) {
            return CAvl_nullref();
        }
    }
    
    ASSERT(CAvl_compare_key_entry(arg, key, c) <= 0);
    
    return c;
}

static CAvlRef CAvl_GetLastLesserEqual (const CAvl *o, CAvlArg arg, CAvlKey key)
{
    CAvlRef c = CAvl_Lookup(o, arg, key);
    if (CAvlIsNullRef(c)) {
        return CAvl_nullref();
    }
    
    if (CAvl_compare_key_entry(arg, key, c) < 0) {
        c = CAvl_GetPrev(o, arg, c);
        if (CAvlIsNullRef(c)) {
            return CAvl_nullref();
        }
    }
    
    ASSERT(CAvl_compare_key_entry(arg, key, c) >= 0);
    
    return c;
}

#endif

static CAvlRef CAvl_GetFirst (const CAvl *o, CAvlArg arg)
{
    if (o->root == CAvl_nulllink()) {
        return CAvl_nullref();
    }
    
    return CAvl_subtree_min(arg, CAvlDeref(arg, o->root));
}

static CAvlRef CAvl_GetLast (const CAvl *o, CAvlArg arg)
{
    if (o->root == CAvl_nulllink()) {
        return CAvl_nullref();
    }
    
    return CAvl_subtree_max(arg, CAvlDeref(arg, o->root));
}

static CAvlRef CAvl_GetNext (const CAvl *o, CAvlArg arg, CAvlRef node)
{
    ASSERT(node.link != CAvl_nulllink())
    ASSERT(o->root != CAvl_nulllink())
    
    if (CAvl_link(node)[1] != CAvl_nulllink()) {
        node = CAvlDeref(arg, CAvl_link(node)[1]);
        while (CAvl_link(node)[0] != CAvl_nulllink()) {
            node = CAvlDeref(arg, CAvl_link(node)[0]);
        }
    } else {
        while (CAvl_parent(node) != CAvl_nulllink() && node.link == CAvl_link(CAvlDeref(arg, CAvl_parent(node)))[1]) {
            node = CAvlDeref(arg, CAvl_parent(node));
        }
        node = CAvlDeref(arg, CAvl_parent(node));
    }
    
    return node;
}

static CAvlRef CAvl_GetPrev (const CAvl *o, CAvlArg arg, CAvlRef node)
{
    ASSERT(node.link != CAvl_nulllink())
    ASSERT(o->root != CAvl_nulllink())
    
    if (CAvl_link(node)[0] != CAvl_nulllink()) {
        node = CAvlDeref(arg, CAvl_link(node)[0]);
        while (CAvl_link(node)[1] != CAvl_nulllink()) {
            node = CAvlDeref(arg, CAvl_link(node)[1]);
        }
    } else {
        while (CAvl_parent(node) != CAvl_nulllink() && node.link == CAvl_link(CAvlDeref(arg, CAvl_parent(node)))[0]) {
            node = CAvlDeref(arg, CAvl_parent(node));
        }
        node = CAvlDeref(arg, CAvl_parent(node));
    }
    
    return node;
}

static int CAvl_IsEmpty (const CAvl *o)
{
    return o->root == CAvl_nulllink();
}

static void CAvl_Verify (const CAvl *o, CAvlArg arg)
{
    if (o->root != CAvl_nulllink()) {
        CAvlRef root = CAvlDeref(arg, o->root);
        ASSERT(CAvl_parent(root) == CAvl_nulllink())
        CAvl_verify_recurser(arg, root);
    }
}

#if CAVL_PARAM_FEATURE_COUNTS

static CAvlCount CAvl_Count (const CAvl *o, CAvlArg arg)
{
    return (o->root != CAvl_nulllink() ? CAvl_count(CAvlDeref(arg, o->root)) : 0);
}

static CAvlCount CAvl_IndexOf (const CAvl *o, CAvlArg arg, CAvlRef node)
{
    ASSERT(node.link != CAvl_nulllink())
    ASSERT(o->root != CAvl_nulllink())
    
    CAvlCount index = (CAvl_link(node)[0] != CAvl_nulllink() ? CAvl_count(CAvlDeref(arg, CAvl_link(node)[0])) : 0);
    
    CAvlRef paren = CAvlDeref(arg, CAvl_parent(node));
    
    for (CAvlRef c = node; paren.link != CAvl_nulllink(); c = paren, paren = CAvlDeref(arg, CAvl_parent(c))) {
        if (c.link == CAvl_link(paren)[1]) {
            ASSERT(CAvl_count(paren) > CAvl_count(c))
            ASSERT(CAvl_count(paren) - CAvl_count(c) <= CAVL_PARAM_VALUE_COUNT_MAX - index)
            index += CAvl_count(paren) - CAvl_count(c);
        }
    }
    
    return index;
}

static CAvlRef CAvl_GetAt (const CAvl *o, CAvlArg arg, CAvlCount index)
{
    if (index >= CAvl_Count(o, arg)) {
        return CAvl_nullref();
    }
    
    CAvlRef c = CAvlDeref(arg, o->root);
    
    while (1) {
        ASSERT(c.link != CAvl_nulllink())
        ASSERT(index < CAvl_count(c))
        
        CAvlCount left_count = (CAvl_link(c)[0] != CAvl_nulllink() ? CAvl_count(CAvlDeref(arg, CAvl_link(c)[0])) : 0);
        
        if (index == left_count) {
            return c;
        }
        
        if (index < left_count) {
            c = CAvlDeref(arg, CAvl_link(c)[0]);
        } else {
            c = CAvlDeref(arg, CAvl_link(c)[1]);
            index -= left_count + 1;
        }
    }
}

#endif

#if CAVL_PARAM_FEATURE_ASSOC

static CAvlAssoc CAvl_AssocSum (const CAvl *o, CAvlArg arg)
{
    if (o->root == CAvl_nulllink()) {
        return CAVL_PARAM_VALUE_ASSOC_ZERO;
    }
    CAvlRef root = CAvlDeref(arg, o->root);
    return CAvl_assoc(root);
}

static CAvlAssoc CAvl_ExclusiveAssocPrefixSum (const CAvl *o, CAvlArg arg, CAvlRef node)
{
    ASSERT(node.link != CAvl_nulllink())
    ASSERT(o->root != CAvl_nulllink())
    
    CAvlAssoc sum = (CAvl_link(node)[0] != CAvl_nulllink() ? CAvl_assoc(CAvlDeref(arg, CAvl_link(node)[0])) : CAVL_PARAM_VALUE_ASSOC_ZERO);
    
    CAvlRef paren = CAvlDeref(arg, CAvl_parent(node));
    
    for (CAvlRef c = node; paren.link != CAvl_nulllink(); c = paren, paren = CAvlDeref(arg, CAvl_parent(c))) {
        if (c.link == CAvl_link(paren)[1]) {
            CAvlAssoc c_val = CAVL_PARAM_FUN_ASSOC_VALUE(arg, paren);
            sum = CAVL_PARAM_FUN_ASSOC_OPER(arg, c_val, sum);
            if (CAvl_link(paren)[0] != CAvl_nulllink()) {
                sum = CAVL_PARAM_FUN_ASSOC_OPER(arg, CAvl_assoc(CAvlDeref(arg, CAvl_link(paren)[0])), sum);
            }
        }
    }
    
    return sum;
}

static CAvlRef CAvl_FindLastExclusiveAssocPrefixSumLesserEqual (const CAvl *o, CAvlArg arg, CAvlAssoc sum, int (*sum_less) (void *, CAvlAssoc, CAvlAssoc), void *user)
{
    CAvlRef result = CAvl_nullref();
    CAvlRef c = CAvlDeref(arg, o->root);
    CAvlAssoc sum_offset = CAVL_PARAM_VALUE_ASSOC_ZERO;
    
    while (c.link != CAvl_nulllink()) {
        CAvlAssoc left_sum = (CAvl_link(c)[0] != CAvl_nulllink() ? CAvl_assoc(CAvlDeref(arg, CAvl_link(c)[0])) : CAVL_PARAM_VALUE_ASSOC_ZERO);
        CAvlAssoc c_prefixsum = CAVL_PARAM_FUN_ASSOC_OPER(arg, sum_offset, left_sum);
        
        if (sum_less(user, sum, c_prefixsum)) {
            c = CAvlDeref(arg, CAvl_link(c)[0]);
        } else {
            result = c;
            CAvlAssoc c_val = CAVL_PARAM_FUN_ASSOC_VALUE(arg, c);
            sum_offset = CAVL_PARAM_FUN_ASSOC_OPER(arg, c_prefixsum, c_val);
            c = CAvlDeref(arg, CAvl_link(c)[1]);
        }
    }
    
    return result;
}

#endif

#include "CAvl_footer.h"
