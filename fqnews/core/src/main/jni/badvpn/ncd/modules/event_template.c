/**
 * @file event_template.c
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

#include <ncd/modules/event_template.h>

#define TemplateLog(o, ...) NCDModuleInst_Backend_Log((o)->i, (o)->blog_channel, __VA_ARGS__)

static void enable_event (event_template *o)
{
    ASSERT(!LinkedList1_IsEmpty(&o->events_list))
    ASSERT(!o->enabled)
    
    // get event
    struct event_template_event *e = UPPER_OBJECT(LinkedList1_GetFirst(&o->events_list), struct event_template_event, events_list_node);
    
    // remove from events list
    LinkedList1_Remove(&o->events_list, &e->events_list_node);
    
    // grab enabled map
    o->enabled_map = e->map;
    
    // append to free list
    LinkedList1_Append(&o->free_list, &e->events_list_node);
    
    // set enabled
    o->enabled = 1;
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
}

void event_template_new (event_template *o, NCDModuleInst *i, int blog_channel, int maxevents, void *user,
                         event_template_func_free func_free)
{
    ASSERT(maxevents > 0)
    
    // init arguments
    o->i = i;
    o->blog_channel = blog_channel;
    o->user = user;
    o->func_free = func_free;
    
    // allocate events array
    if (!(o->events = BAllocArray(maxevents, sizeof(o->events[0])))) {
        TemplateLog(o, BLOG_ERROR, "BAllocArray failed");
        goto fail0;
    }
    
    // init events lists
    LinkedList1_Init(&o->events_list);
    LinkedList1_Init(&o->free_list);
    for (int j = 0; j < maxevents; j++) {
        LinkedList1_Append(&o->free_list, &o->events[j].events_list_node);
    }
    
    // set not enabled
    o->enabled = 0;
    
    return;
    
fail0:
    o->func_free(o->user, 1);
    return;
}

void event_template_die (event_template *o)
{
    // free enabled map
    if (o->enabled) {
        BStringMap_Free(&o->enabled_map);
    }
    
    // free event maps
    LinkedList1Node *list_node = LinkedList1_GetFirst(&o->events_list);
    while (list_node) {
        struct event_template_event *e = UPPER_OBJECT(list_node, struct event_template_event, events_list_node);
        BStringMap_Free(&e->map);
        list_node = LinkedList1Node_Next(list_node);
    }
    
    // free events array
    BFree(o->events);
    
    o->func_free(o->user, 0);
    return;
}

int event_template_getvar (event_template *o, const char *name, NCDValMem *mem, NCDValRef *out)
{
    ASSERT(o->enabled)
    ASSERT(name)
    
    const char *val = BStringMap_Get(&o->enabled_map, name);
    if (!val) {
        return 0;
    }
    
    *out = NCDVal_NewString(mem, val);
    return 1;
}

void event_template_queue (event_template *o, BStringMap map, int *out_was_empty)
{
    ASSERT(!LinkedList1_IsEmpty(&o->free_list))
    
    // get event
    struct event_template_event *e = UPPER_OBJECT(LinkedList1_GetFirst(&o->free_list), struct event_template_event, events_list_node);
    
    // remove from free list
    LinkedList1_Remove(&o->free_list, &e->events_list_node);
    
    // set map
    e->map = map;
    
    // insert to events list
    LinkedList1_Append(&o->events_list, &e->events_list_node);
    
    // enable if not already
    if (!o->enabled) {
        enable_event(o);
        *out_was_empty = 1;
    } else {
        *out_was_empty = 0;
    }
}

void event_template_dequeue (event_template *o, int *out_is_empty)
{
    ASSERT(o->enabled)
    
    // free enabled map
    BStringMap_Free(&o->enabled_map);
    
    // set not enabled
    o->enabled = 0;
    
    // signal down
    NCDModuleInst_Backend_Down(o->i);
    
    // enable if there are more events
    if (!LinkedList1_IsEmpty(&o->events_list)) {
        enable_event(o);
        *out_is_empty = 0;
    } else {
        *out_is_empty = 1;
    }
}

int event_template_is_enabled (event_template *o)
{
    return o->enabled;
}
