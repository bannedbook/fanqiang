/**
 * @file NCDUdevManager.c
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
#include <base/BLog.h>

#include <udevmonitor/NCDUdevManager.h>

#include <generated/blog_channel_NCDUdevManager.h>

#define RESTART_TIMER_TIME 5000

static int event_to_map (NCDUdevMonitor *monitor, BStringMap *out_map);
static void free_event (NCDUdevClient *o, struct NCDUdevClient_event *e);
static void queue_event (NCDUdevManager *o, NCDUdevMonitor *monitor, NCDUdevClient *client);
static void queue_mapless_event (NCDUdevManager *o, const char *devpath, NCDUdevClient *client);
static void process_event (NCDUdevManager *o, NCDUdevMonitor *monitor);
static void try_monitor (NCDUdevManager *o);
static void reset_monitor (NCDUdevManager *o);
static void timer_handler (NCDUdevManager *o);
static void monitor_handler_event (NCDUdevManager *o);
static void monitor_handler_error (NCDUdevManager *o, int is_error);
static void info_monitor_handler_event (NCDUdevManager *o);
static void info_monitor_handler_error (NCDUdevManager *o, int is_error);
static void next_job_handler (NCDUdevClient *o);

static int event_to_map (NCDUdevMonitor *monitor, BStringMap *out_map)
{
    NCDUdevMonitor_AssertReady(monitor);
    
    // init map
    BStringMap_Init(out_map);
    
    // insert properties to map
    int num_properties = NCDUdevMonitor_GetNumProperties(monitor);
    for (int i = 0; i < num_properties; i++) {
        const char *name;
        const char *value;
        NCDUdevMonitor_GetProperty(monitor, i, &name, &value);
        
        if (!BStringMap_Set(out_map, name, value)) {
            BLog(BLOG_ERROR, "BStringMap_Set failed");
            goto fail1;
        }
    }
    
    return 1;
    
fail1:
    BStringMap_Free(out_map);
    return 0;
}

static void free_event (NCDUdevClient *o, struct NCDUdevClient_event *e)
{
    // remove from events list
    LinkedList1_Remove(&o->events_list, &e->events_list_node);
    
    // free map
    if (e->have_map) {
        BStringMap_Free(&e->map);
    }
    
    // free devpath
    free(e->devpath);
    
    // free structure
    free(e);
}

static void queue_event (NCDUdevManager *o, NCDUdevMonitor *monitor, NCDUdevClient *client)
{
    NCDUdevMonitor_AssertReady(monitor);
    
    // alloc event
    struct NCDUdevClient_event *e = malloc(sizeof(*e));
    if (!e) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // build map
    if (!event_to_map(monitor, &e->map)) {
        goto fail1;
    }
    
    // set have map
    e->have_map = 1;
    
    // get devpath
    const char *devpath = BStringMap_Get(&e->map, "DEVPATH");
    if (!devpath) {
        BLog(BLOG_ERROR, "DEVPATH missing");
        goto fail2;
    }
    
    // copy devpath
    if (!(e->devpath = strdup(devpath))) {
        BLog(BLOG_ERROR, "strdup failed");
        goto fail2;
    }
    
    // insert to client's events list
    LinkedList1_Append(&client->events_list, &e->events_list_node);
    
    // if client is running, set next job
    if (client->running) {
        BPending_Set(&client->next_job);
    }
    
    return;
    
fail2:
    BStringMap_Free(&e->map);
fail1:
    free(e);
fail0:
    return;
}

static void queue_mapless_event (NCDUdevManager *o, const char *devpath, NCDUdevClient *client)
{
    // alloc event
    struct NCDUdevClient_event *e = malloc(sizeof(*e));
    if (!e) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // set have no map
    e->have_map = 0;
    
    // copy devpath
    if (!(e->devpath = strdup(devpath))) {
        BLog(BLOG_ERROR, "strdup failed");
        goto fail1;
    }
    
    // insert to client's events list
    LinkedList1_Append(&client->events_list, &e->events_list_node);
    
    // if client is running, set next job
    if (client->running) {
        BPending_Set(&client->next_job);
    }
    
    return;
    
fail1:
    free(e);
fail0:
    return;
}

static void process_event (NCDUdevManager *o, NCDUdevMonitor *monitor)
{
    NCDUdevMonitor_AssertReady(monitor);
    
    // build map from event
    BStringMap map;
    if (!event_to_map(monitor, &map)) {
        BLog(BLOG_ERROR, "failed to build map");
        return;
    }
    
    // pass event to cache
    if (!NCDUdevCache_Event(&o->cache, map)) {
        BLog(BLOG_ERROR, "failed to cache");
        BStringMap_Free(&map);
        return;
    }
    
    // queue event to clients
    LinkedList1Node *list_node = LinkedList1_GetFirst(&o->clients_list);
    while (list_node) {
        NCDUdevClient *client = UPPER_OBJECT(list_node, NCDUdevClient, clients_list_node);
        queue_event(o, monitor, client);
        list_node = LinkedList1Node_Next(list_node);
    }
}

static void try_monitor (NCDUdevManager *o)
{
    ASSERT(!o->have_monitor)
    ASSERT(!BTimer_IsRunning(&o->restart_timer))
    
    int mode = (o->no_udev ? NCDUDEVMONITOR_MODE_MONITOR_KERNEL : NCDUDEVMONITOR_MODE_MONITOR_UDEV);
    
    // init monitor
    if (!NCDUdevMonitor_Init(&o->monitor, o->reactor, o->manager, mode, o,
        (NCDUdevMonitor_handler_event)monitor_handler_event,
        (NCDUdevMonitor_handler_error)monitor_handler_error
    )) {
        BLog(BLOG_ERROR, "NCDUdevMonitor_Init failed");
        
        // set restart timer
        BReactor_SetTimer(o->reactor, &o->restart_timer);
        return;
    }
    
    // set have monitor
    o->have_monitor = 1;
    
    // set not have info monitor
    o->have_info_monitor = 0;
}

static void reset_monitor (NCDUdevManager *o)
{
    ASSERT(o->have_monitor)
    ASSERT(!o->have_info_monitor)
    ASSERT(!BTimer_IsRunning(&o->restart_timer))
    
    // free monitor
    NCDUdevMonitor_Free(&o->monitor);
    
    // set have no monitor
    o->have_monitor = 0;
    
    // set restart timer
    BReactor_SetTimer(o->reactor, &o->restart_timer);
}

static void timer_handler (NCDUdevManager *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->have_monitor)
    
    // try again
    try_monitor(o);
}

static void monitor_handler_event (NCDUdevManager *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_monitor)
    ASSERT(!o->have_info_monitor)
    ASSERT(!BTimer_IsRunning(&o->restart_timer))
    
    if (NCDUdevMonitor_IsReadyEvent(&o->monitor)) {
        BLog(BLOG_INFO, "monitor ready");
        
        // init info monitor
        if (!NCDUdevMonitor_Init(&o->info_monitor, o->reactor, o->manager, NCDUDEVMONITOR_MODE_INFO, o,
            (NCDUdevMonitor_handler_event)info_monitor_handler_event,
            (NCDUdevMonitor_handler_error)info_monitor_handler_error
        )) {
            BLog(BLOG_ERROR, "NCDUdevMonitor_Init failed");
            reset_monitor(o);
            return;
        }
        
        // set have info monitor
        o->have_info_monitor = 1;
        
        // start cache cleanup
        NCDUdevCache_StartClean(&o->cache);
        
        // hold processing monitor events until info monitor is done
        return;
    }
    
    // accept event
    NCDUdevMonitor_Done(&o->monitor);
    
    // process event
    process_event(o, &o->monitor);
}

static void monitor_handler_error (NCDUdevManager *o, int is_error)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_monitor)
    ASSERT(!BTimer_IsRunning(&o->restart_timer))
    
    BLog(BLOG_ERROR, "monitor error");
    
    if (o->have_info_monitor) {
        // free info monitor
        NCDUdevMonitor_Free(&o->info_monitor);
        
        // set have no info monitor
        o->have_info_monitor = 0;
    }
    
    // reset monitor
    reset_monitor(o);
}

static void info_monitor_handler_event (NCDUdevManager *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_monitor)
    ASSERT(o->have_info_monitor)
    ASSERT(!BTimer_IsRunning(&o->restart_timer))
    
    // accept event
    NCDUdevMonitor_Done(&o->info_monitor);
    
    // process event
    process_event(o, &o->info_monitor);
}

static void info_monitor_handler_error (NCDUdevManager *o, int is_error)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->have_monitor)
    ASSERT(o->have_info_monitor)
    ASSERT(!BTimer_IsRunning(&o->restart_timer))
    
    if (is_error) {
        BLog(BLOG_ERROR, "info monitor error");
    } else {
        BLog(BLOG_INFO, "info monitor finished");
    }
    
    // free info monitor
    NCDUdevMonitor_Free(&o->info_monitor);
    
    // set have no info monitor
    o->have_info_monitor = 0;
    
    if (is_error) {
        // reset monitor
        reset_monitor(o);
    } else {
        // continue processing monitor events
        NCDUdevMonitor_Done(&o->monitor);
        
        // finish cache cleanup
        NCDUdevCache_FinishClean(&o->cache);
        
        // collect cleaned devices
        BStringMap map;
        while (NCDUdevCache_GetCleanedDevice(&o->cache, &map)) {
            // get devpath
            const char *devpath = BStringMap_Get(&map, "DEVPATH");
            ASSERT(devpath)
            
            // queue mapless event to clients
            LinkedList1Node *list_node = LinkedList1_GetFirst(&o->clients_list);
            while (list_node) {
                NCDUdevClient *client = UPPER_OBJECT(list_node, NCDUdevClient, clients_list_node);
                queue_mapless_event(o, devpath, client);
                list_node = LinkedList1Node_Next(list_node);
            }
            
            BStringMap_Free(&map);
        }
    }
}

static void next_job_handler (NCDUdevClient *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!LinkedList1_IsEmpty(&o->events_list))
    ASSERT(o->running)
    
    // get event
    struct NCDUdevClient_event *e = UPPER_OBJECT(LinkedList1_GetFirst(&o->events_list), struct NCDUdevClient_event, events_list_node);
    
    // grab map from event
    int have_map = e->have_map;
    BStringMap map = e->map;
    
    // grab devpath from event
    char *devpath = e->devpath;
    
    // remove from events list
    LinkedList1_Remove(&o->events_list, &e->events_list_node);
    
    // free structure
    free(e);
    
    // schedule next event if needed
    if (!LinkedList1_IsEmpty(&o->events_list)) {
        BPending_Set(&o->next_job);
    }
    
    // give map to handler
    o->handler(o->user, devpath, have_map, map);
    return;
}

void NCDUdevManager_Init (NCDUdevManager *o, int no_udev, BReactor *reactor, BProcessManager *manager)
{
    ASSERT(no_udev == 0 || no_udev == 1)
    
    // init arguments
    o->no_udev = no_udev;
    o->reactor = reactor;
    o->manager = manager;
    
    // init clients list
    LinkedList1_Init(&o->clients_list);
    
    // init cache
    NCDUdevCache_Init(&o->cache);
    
    // init restart timer
    BTimer_Init(&o->restart_timer, RESTART_TIMER_TIME, (BTimer_handler)timer_handler, o);
    
    // set have no monitor
    o->have_monitor = 0;
    
    DebugObject_Init(&o->d_obj);
}

void NCDUdevManager_Free (NCDUdevManager *o)
{
    DebugObject_Free(&o->d_obj);
    ASSERT(LinkedList1_IsEmpty(&o->clients_list))
    
    if (o->have_monitor) {
        // free info monitor
        if (o->have_info_monitor) {
            NCDUdevMonitor_Free(&o->info_monitor);
        }
        
        // free monitor
        NCDUdevMonitor_Free(&o->monitor);
    }
    
    // free restart timer
    BReactor_RemoveTimer(o->reactor, &o->restart_timer);
    
    // free cache
    NCDUdevCache_Free(&o->cache);
}

const BStringMap * NCDUdevManager_Query (NCDUdevManager *o, const char *devpath)
{
    DebugObject_Access(&o->d_obj);
    
    return NCDUdevCache_Query(&o->cache, devpath);
}

void NCDUdevClient_Init (NCDUdevClient *o, NCDUdevManager *m, void *user,
                         NCDUdevClient_handler handler)
{
    DebugObject_Access(&m->d_obj);
    
    // init arguments
    o->m = m;
    o->user = user;
    o->handler = handler;
    
    // insert to manager's list
    LinkedList1_Append(&m->clients_list, &o->clients_list_node);
    
    // init events list
    LinkedList1_Init(&o->events_list);
    
    // init next job
    BPending_Init(&o->next_job, BReactor_PendingGroup(m->reactor), (BPending_handler)next_job_handler, o);
    
    // set running
    o->running = 1;
    
    // queue all devices from cache
    const char *devpath = NCDUdevCache_First(&m->cache);
    while (devpath) {
        queue_mapless_event(m, devpath, o);
        devpath = NCDUdevCache_Next(&m->cache, devpath);
    }
    
    // if this is the first client, init monitor
    if (!m->have_monitor && !BTimer_IsRunning(&m->restart_timer)) {
        try_monitor(m);
    }
    
    DebugObject_Init(&o->d_obj);
}

void NCDUdevClient_Free (NCDUdevClient *o)
{
    DebugObject_Free(&o->d_obj);
    NCDUdevManager *m = o->m;
    
    // free events
    LinkedList1Node *list_node;
    while (list_node = LinkedList1_GetFirst(&o->events_list)) {
        struct NCDUdevClient_event *e = UPPER_OBJECT(list_node, struct NCDUdevClient_event, events_list_node);
        free_event(o, e);
    }
    
    // free next job
    BPending_Free(&o->next_job);
    
    // remove from manager's list
    LinkedList1_Remove(&m->clients_list, &o->clients_list_node);
}

void NCDUdevClient_Pause (NCDUdevClient *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->running)
    
    // set not running
    o->running = 0;
    
    // unset next job to avoid reporting queued events
    BPending_Unset(&o->next_job);
}

void NCDUdevClient_Continue (NCDUdevClient *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->running)
    
    // set running
    o->running = 1;
    
    // set next job if we have events queued
    if (!LinkedList1_IsEmpty(&o->events_list)) {
        BPending_Set(&o->next_job);
    }
}
