/**
 * @file NCDUdevManager.h
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

#ifndef BADVPN_UDEVMONITOR_NCDUDEVMANAGER_H
#define BADVPN_UDEVMONITOR_NCDUDEVMANAGER_H

#include <misc/debug.h>
#include <structure/LinkedList1.h>
#include <base/DebugObject.h>
#include <udevmonitor/NCDUdevMonitor.h>
#include <udevmonitor/NCDUdevCache.h>
#include <stringmap/BStringMap.h>

typedef void (*NCDUdevClient_handler) (void *user, char *devpath, int have_map, BStringMap map);

typedef struct {
    int no_udev;
    BReactor *reactor;
    BProcessManager *manager;
    LinkedList1 clients_list;
    NCDUdevCache cache;
    BTimer restart_timer;
    int have_monitor;
    NCDUdevMonitor monitor;
    int have_info_monitor;
    NCDUdevMonitor info_monitor;
    DebugObject d_obj;
} NCDUdevManager;

typedef struct {
    NCDUdevManager *m;
    void *user;
    NCDUdevClient_handler handler;
    LinkedList1Node clients_list_node;
    LinkedList1 events_list;
    BPending next_job;
    int running;
    DebugObject d_obj;
} NCDUdevClient;

struct NCDUdevClient_event {
    char *devpath;
    int have_map;
    BStringMap map;
    LinkedList1Node events_list_node;
};

void NCDUdevManager_Init (NCDUdevManager *o, int no_udev, BReactor *reactor, BProcessManager *manager);
void NCDUdevManager_Free (NCDUdevManager *o);
const BStringMap * NCDUdevManager_Query (NCDUdevManager *o, const char *devpath);

void NCDUdevClient_Init (NCDUdevClient *o, NCDUdevManager *m, void *user,
                         NCDUdevClient_handler handler);
void NCDUdevClient_Free (NCDUdevClient *o);
void NCDUdevClient_Pause (NCDUdevClient *o);
void NCDUdevClient_Continue (NCDUdevClient *o);

#endif
