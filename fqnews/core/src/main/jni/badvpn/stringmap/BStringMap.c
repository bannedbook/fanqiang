/**
 * @file BStringMap.c
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
#include <string.h>

#include <misc/offset.h>
#include <misc/compare.h>

#include <stringmap/BStringMap.h>

static int string_comparator (void *unused, char **str1, char **str2)
{
    int c = strcmp(*str1, *str2);
    return B_COMPARE(c, 0);
}

static void free_entry (BStringMap *o, struct BStringMap_entry *e)
{
    BAVL_Remove(&o->tree, &e->tree_node);
    free(e->value);
    free(e->key);
    free(e);
}

void BStringMap_Init (BStringMap *o)
{
    // init tree
    BAVL_Init(&o->tree, OFFSET_DIFF(struct BStringMap_entry, key, tree_node), (BAVL_comparator)string_comparator, NULL);
    
    DebugObject_Init(&o->d_obj);
}

int BStringMap_InitCopy (BStringMap *o, const BStringMap *src)
{
    BStringMap_Init(o);
    
    const char *key = BStringMap_First(src);
    while (key) {
        if (!BStringMap_Set(o, key, BStringMap_Get(src, key))) {
            goto fail1;
        }
        key = BStringMap_Next(src, key);
    }
    
    return 1;
    
fail1:
    BStringMap_Free(o);
    return 0;
}

void BStringMap_Free (BStringMap *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free entries
    BAVLNode *tree_node;
    while (tree_node = BAVL_GetFirst(&o->tree)) {
        struct BStringMap_entry *e = UPPER_OBJECT(tree_node, struct BStringMap_entry, tree_node);
        free_entry(o, e);
    }
}

const char * BStringMap_Get (const BStringMap *o, const char *key)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(key)
    
    // lookup
    BAVLNode *tree_node = BAVL_LookupExact(&o->tree, &key);
    if (!tree_node) {
        return NULL;
    }
    struct BStringMap_entry *e = UPPER_OBJECT(tree_node, struct BStringMap_entry, tree_node);
    
    return e->value;
}

int BStringMap_Set (BStringMap *o, const char *key, const char *value)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(key)
    ASSERT(value)
    
    // alloc entry
    struct BStringMap_entry *e = malloc(sizeof(*e));
    if (!e) {
        goto fail0;
    }
    
    // alloc and set key
    if (!(e->key = malloc(strlen(key) + 1))) {
        goto fail1;
    }
    strcpy(e->key, key);
    
    // alloc and set value
    if (!(e->value = malloc(strlen(value) + 1))) {
        goto fail2;
    }
    strcpy(e->value, value);
    
    // try inserting to tree
    BAVLNode *ex_tree_node;
    if (!BAVL_Insert(&o->tree, &e->tree_node, &ex_tree_node)) {
        // remove existing entry
        struct BStringMap_entry *ex_e = UPPER_OBJECT(ex_tree_node, struct BStringMap_entry, tree_node);
        free_entry(o, ex_e);
        
        // insert new node
        ASSERT_EXECUTE(BAVL_Insert(&o->tree, &e->tree_node, NULL))
    }
    
    return 1;
    
fail2:
    free(e->key);
fail1:
    free(e);
fail0:
    return 0;
}

void BStringMap_Unset (BStringMap *o, const char *key)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(key)
    
    // lookup
    BAVLNode *tree_node = BAVL_LookupExact(&o->tree, &key);
    if (!tree_node) {
        return;
    }
    struct BStringMap_entry *e = UPPER_OBJECT(tree_node, struct BStringMap_entry, tree_node);
    
    // remove
    free_entry(o, e);
}

const char * BStringMap_First (const BStringMap *o)
{
    DebugObject_Access(&o->d_obj);
    
    // get first
    BAVLNode *tree_node = BAVL_GetFirst(&o->tree);
    if (!tree_node) {
        return NULL;
    }
    struct BStringMap_entry *e = UPPER_OBJECT(tree_node, struct BStringMap_entry, tree_node);
    
    return e->key;
}

const char * BStringMap_Next (const BStringMap *o, const char *key)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(key)
    ASSERT(BAVL_LookupExact(&o->tree, &key))
    
    // get entry
    struct BStringMap_entry *e = UPPER_OBJECT(BAVL_LookupExact(&o->tree, &key), struct BStringMap_entry, tree_node);
    
    // get next
    BAVLNode *tree_node = BAVL_GetNext(&o->tree, &e->tree_node);
    if (!tree_node) {
        return NULL;
    }
    struct BStringMap_entry *next_e = UPPER_OBJECT(tree_node, struct BStringMap_entry, tree_node);
    
    return next_e->key;
}
