/**
 * @file NCDUdevCache.h
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

#ifndef BADVPN_UDEVMONITOR_NCDUDEVCACHE_H
#define BADVPN_UDEVMONITOR_NCDUDEVCACHE_H

#include <misc/debug.h>
#include <structure/BAVL.h>
#include <structure/LinkedList1.h>
#include <base/DebugObject.h>
#include <stringmap/BStringMap.h>

struct NCDUdevCache_device {
    BStringMap map;
    const char *devpath;
    int is_cleaned;
    union {
        BAVLNode devices_tree_node;
        LinkedList1Node cleaned_devices_list_node;
    };
    int is_refreshed;
};

typedef struct {
    BAVL devices_tree;
    LinkedList1 cleaned_devices_list;
    DebugObject d_obj;
} NCDUdevCache;

void NCDUdevCache_Init (NCDUdevCache *o);
void NCDUdevCache_Free (NCDUdevCache *o);
const BStringMap * NCDUdevCache_Query (NCDUdevCache *o, const char *devpath);
int NCDUdevCache_Event (NCDUdevCache *o, BStringMap map) WARN_UNUSED;
void NCDUdevCache_StartClean (NCDUdevCache *o);
void NCDUdevCache_FinishClean (NCDUdevCache *o);
int NCDUdevCache_GetCleanedDevice (NCDUdevCache *o, BStringMap *out_map);
const char * NCDUdevCache_First (NCDUdevCache *o);
const char * NCDUdevCache_Next (NCDUdevCache *o, const char *key);

#endif
