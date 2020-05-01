/**
 * @file event_template.h
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

#ifndef BADVPN_NCD_MODULES_EVENT_TEMPLATE_H
#define BADVPN_NCD_MODULES_EVENT_TEMPLATE_H

#include <structure/LinkedList1.h>
#include <stringmap/BStringMap.h>
#include <ncd/NCDModule.h>

typedef void (*event_template_func_free) (void *user, int is_error);

typedef struct {
    NCDModuleInst *i;
    int blog_channel;
    void *user;
    event_template_func_free func_free;
    struct event_template_event *events;
    LinkedList1 events_list;
    LinkedList1 free_list;
    int enabled;
    BStringMap enabled_map;
} event_template;

struct event_template_event {
    BStringMap map;
    LinkedList1Node events_list_node;
};

void event_template_new (event_template *o, NCDModuleInst *i, int blog_channel, int maxevents, void *user,
                         event_template_func_free func_free);
void event_template_die (event_template *o);
int event_template_getvar (event_template *o, const char *name, NCDValMem *mem, NCDValRef *out);
void event_template_queue (event_template *o, BStringMap map, int *out_was_empty);
void event_template_dequeue (event_template *o, int *out_is_empty);
int event_template_is_enabled (event_template *o);

#endif
