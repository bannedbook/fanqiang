/**
 * @file net_backend_rfkill.c
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
 * Rfkill monitoring module.
 * 
 * Synopsis: net.backend.rfkill(string type, string name)
 * Arguments:
 *   type - method of determining the index of the rfkill device. "index" for
 *     rfkill device index, "wlan" for wireless device. Be aware that, for
 *     the wireless device method, the index is resloved at initialization,
 *     and no attempt is made to refresh it if the device goes away. In other
 *     words, you should probably put a "net.backend.waitdevice" statement
 *     in front of the rfkill statement.
 *   name - rfkill index or wireless device name
 */

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include <misc/string_begins_with.h>
#include <ncd/extra/NCDRfkillMonitor.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_net_backend_rfkill.h>

struct instance {
    NCDModuleInst *i;
    uint32_t index;
    NCDRfkillMonitor monitor;
    int up;
};

static int find_wlan_rfill (const char *ifname, uint32_t *out_index)
{
    char ieee_path[100];
    snprintf(ieee_path, sizeof(ieee_path), "/sys/class/net/%s/../../ieee80211", ifname);
    
    int res = 0;
    
    DIR *d = opendir(ieee_path);
    if (!d) {
        goto fail0;
    }
    
    struct dirent *e;
    while (e = readdir(d)) {
        if (!string_begins_with(e->d_name, "phy")) {
            continue;
        }
        
        char phy_path[150];
        snprintf(phy_path, sizeof(phy_path), "%s/%s", ieee_path, e->d_name);
        
        DIR *d2 = opendir(phy_path);
        if (!d2) {
            continue;
        }
        
        struct dirent *e2;
        while (e2 = readdir(d2)) {
            int index_pos;
            if (!(index_pos = string_begins_with(e2->d_name, "rfkill"))) {
                continue;
            }
            
            uint32_t index;
            if (sscanf(e2->d_name + index_pos, "%"SCNu32, &index) != 1) {
                continue;
            }
            
            res = 1;
            *out_index = index;
        }
        
        closedir(d2);
    }
    
    closedir(d);
fail0:
    return res;
}

static void monitor_handler (struct instance *o, struct rfkill_event event)
{
    if (event.idx != o->index) {
        return;
    }
    
    int was_up = o->up;
    o->up = (event.op != RFKILL_OP_DEL && !event.soft && !event.hard);
    
    if (o->up && !was_up) {
        NCDModuleInst_Backend_Up(o->i);
    }
    else if (!o->up && was_up) {
        NCDModuleInst_Backend_Down(o->i);
    }
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef type_arg;
    NCDValRef name_arg;
    if (!NCDVal_ListRead(params->args, 2, &type_arg, &name_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(type_arg) || !NCDVal_IsStringNoNulls(name_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // null terminate name
    NCDValNullTermString name_nts;
    if (!NCDVal_StringNullTerminate(name_arg, &name_nts)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    if (NCDVal_StringEquals(type_arg, "index")) {
        if (sscanf(name_nts.data, "%"SCNu32, &o->index) != 1) {
            ModuleLog(o->i, BLOG_ERROR, "wrong index argument");
            goto fail1;
        }
    }
    else if (NCDVal_StringEquals(type_arg, "wlan")) {
        if (!find_wlan_rfill(name_nts.data, &o->index)) {
            ModuleLog(o->i, BLOG_ERROR, "failed to find rfkill for wlan interface");
            goto fail1;
        }
    }
    else {
        ModuleLog(o->i, BLOG_ERROR, "unknown type argument");
        goto fail1;
    }
    
    // init monitor
    if (!NCDRfkillMonitor_Init(&o->monitor, o->i->params->iparams->reactor, (NCDRfkillMonitor_handler)monitor_handler, o)) {
        ModuleLog(o->i, BLOG_ERROR, "monitor failed");
        goto fail1;
    }
    
    // set not up
    o->up = 0;
    
    // free name nts
    NCDValNullTermString_Free(&name_nts);
    return;
    
fail1:
    NCDValNullTermString_Free(&name_nts);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    // free monitor
    NCDRfkillMonitor_Free(&o->monitor);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static struct NCDModule modules[] = {
    {
        .type = "net.backend.rfkill",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_net_backend_rfkill = {
    .modules = modules
};
