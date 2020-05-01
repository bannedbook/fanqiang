/**
 * @file indexedlist_test.c
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

#include <misc/debug.h>
#include <misc/offset.h>
#include <structure/IndexedList.h>

IndexedList il;

struct elem {
    int value;
    IndexedListNode node;
};

static void elem_insert (struct elem *e, int value, uint64_t index)
{
    e->value = value;
    IndexedList_InsertAt(&il, &e->node, index);
}

static void remove_at (uint64_t index)
{
    IndexedListNode *n = IndexedList_GetAt(&il, index);
    struct elem *e = UPPER_OBJECT(n, struct elem, node);
    IndexedList_Remove(&il, &e->node);
}

static void print_list (void)
{
    for (uint64_t i = 0; i < IndexedList_Count(&il); i++) {
        IndexedListNode *n = IndexedList_GetAt(&il, i);
        struct elem *e = UPPER_OBJECT(n, struct elem, node);
        printf("%d ", e->value);
    }
    printf("\n");
}

int main (int argc, char *argv[])
{
    IndexedList_Init(&il);
    
    struct elem arr[100];
    
    print_list();
    
    elem_insert(&arr[0], 1, 0);
    print_list();
    elem_insert(&arr[1], 2, 0);
    print_list();
    elem_insert(&arr[2], 3, 0);
    print_list();
    elem_insert(&arr[3], 4, 0);
    print_list();
    elem_insert(&arr[4], 5, 0);
    print_list();
    elem_insert(&arr[5], 6, 0);
    print_list();
    
    elem_insert(&arr[6], 7, 1);
    print_list();
    
    remove_at(0);
    print_list();
    
    remove_at(5);
    print_list();
    
    return 0;
}
