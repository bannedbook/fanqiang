/**
 * @file bavl_test.c
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

#include <stdlib.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <misc/balloc.h>
#include <misc/compare.h>
#include <structure/BAVL.h>
#include <security/BRandom.h>

struct mynode {
    int used;
    int num;
    BAVLNode avl_node;
};

static int int_comparator (void *user, int *val1, int *val2)
{
    return B_COMPARE(*val1, *val2);
}

static void verify (BAVL *tree)
{
    printf("Verifying...\n");
    BAVL_Verify(tree);
}

int main (int argc, char **argv)
{
    int num_nodes;
    int num_random_delete;
    
    if (argc != 3 || (num_nodes = atoi(argv[1])) <= 0 || (num_random_delete = atoi(argv[2])) < 0) {
        fprintf(stderr, "Usage: %s <num> <numrandomdelete>\n", (argc > 0 ? argv[0] : NULL));
        return 1;
    }
    
    struct mynode *nodes = (struct mynode *)BAllocArray(num_nodes, sizeof(*nodes));
    ASSERT_FORCE(nodes)
    
    int *values_ins = (int *)BAllocArray(num_nodes, sizeof(int));
    ASSERT_FORCE(values_ins)
    
    int *values = (int *)BAllocArray(num_random_delete, sizeof(int));
    ASSERT_FORCE(values)
    
    BAVL avl;
    BAVL_Init(&avl, OFFSET_DIFF(struct mynode, num, avl_node), (BAVL_comparator)int_comparator, NULL);
    verify(&avl);
    
    printf("Inserting random values...\n");
    int inserted = 0;
    BRandom_randomize((uint8_t *)values_ins, num_nodes * sizeof(int));
    for (int i = 0; i < num_nodes; i++) {
        nodes[i].num = values_ins[i];
        if (BAVL_Insert(&avl, &nodes[i].avl_node, NULL)) {
            nodes[i].used = 1;
            inserted++;
        } else {
            nodes[i].used = 0;
            printf("Insert collision!\n");
        }
    }
    printf("Inserted %d entries\n", inserted);
    verify(&avl);
    
    printf("Removing random entries...\n");
    int removed1 = 0;
    BRandom_randomize((uint8_t *)values, num_random_delete * sizeof(int));
    for (int i = 0; i < num_random_delete; i++) {
        int index = (((unsigned int *)values)[i] % num_nodes);
        struct mynode *node = nodes + index;
        if (node->used) {
            BAVL_Remove(&avl, &node->avl_node);
            node->used = 0;
            removed1++;
        }
    }
    printf("Removed %d entries\n", removed1);
    verify(&avl);
    
    printf("Removing remaining...\n");
    int removed2 = 0;
    while (!BAVL_IsEmpty(&avl)) {
        struct mynode *node = UPPER_OBJECT(BAVL_GetFirst(&avl), struct mynode, avl_node);
        ASSERT_FORCE(node->used)
        BAVL_Remove(&avl, &node->avl_node);
        node->used = 0;
        removed2++;
    }
    printf("Removed %d entries\n", removed2);
    ASSERT_FORCE(BAVL_IsEmpty(&avl))
    ASSERT_FORCE(removed1 + removed2 == inserted)
    verify(&avl);
    
    BFree(nodes);
    BFree(values_ins);
    BFree(values);
    
    return 0;
}
