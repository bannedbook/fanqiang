/**
 * @file net_watch_interfaces.c
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
 * Network interface watcher.
 * 
 * Synopsis: net.watch_interfaces()
 * Description: reports network interface events. Transitions up when an event is detected, and
 *   goes down waiting for the next event when net.watch_interfaces::nextevent() is called.
 *   On startup, "added" events are reported for existing interfaces.
 * Variables:
 *   string event_type - what happened with the interface: "added" or "removed". This may not be
 *     consistent across events.
 *   string devname - interface name
 *   string bus - bus location, for example "pci:0000:06:00.0", "usb:2-1.3:1.0", or "unknown"
 * 
 * Synopsis: net.watch_interfaces::nextevent()
 * Description: makes the watch_interfaces module transition down in order to report the next event.
 */

#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include <misc/debug.h>
#include <misc/offset.h>
#include <misc/parse_number.h>
#include <misc/bsize.h>
#include <structure/LinkedList1.h>
#include <udevmonitor/NCDUdevManager.h>
#include <ncd/modules/event_template.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_net_watch_interfaces.h>

#define DEVPATH_REGEX "/net/[^/]+$"
#define DEVPATH_USB_REGEX "/usb[^/]*(/[^/]+)+/([^/]+)/net/[^/]+$"
#define DEVPATH_PCI_REGEX "/pci[^/]*/[^/]+/([^/]+)/net/[^/]+$"

struct device {
    char *ifname;
    char *devpath;
    uintmax_t ifindex;
    BStringMap removed_map;
    LinkedList1Node devices_list_node;
};

struct instance {
    NCDModuleInst *i;
    NCDUdevClient client;
    LinkedList1 devices_list;
    regex_t reg;
    regex_t usb_reg;
    regex_t pci_reg;
    event_template templ;
};

static void templ_func_free (struct instance *o, int is_error);

static struct device * find_device_by_ifname (struct instance *o, const char *ifname)
{
    LinkedList1Node *list_node = LinkedList1_GetFirst(&o->devices_list);
    while (list_node) {
        struct device *device = UPPER_OBJECT(list_node, struct device, devices_list_node);
        if (!strcmp(device->ifname, ifname)) {
            return device;
        }
        list_node = LinkedList1Node_Next(list_node);
    }
    
    return NULL;
}

static struct device * find_device_by_devpath (struct instance *o, const char *devpath)
{
    LinkedList1Node *list_node = LinkedList1_GetFirst(&o->devices_list);
    while (list_node) {
        struct device *device = UPPER_OBJECT(list_node, struct device, devices_list_node);
        if (!strcmp(device->devpath, devpath)) {
            return device;
        }
        list_node = LinkedList1Node_Next(list_node);
    }
    
    return NULL;
}

static void free_device (struct instance *o, struct device *device, int have_removed_map)
{
    // remove from devices list
    LinkedList1_Remove(&o->devices_list, &device->devices_list_node);
    
    // free removed map
    if (have_removed_map) {
        BStringMap_Free(&device->removed_map);
    }
    
    // free devpath
    free(device->devpath);
    
    // free ifname
    free(device->ifname);
    
    // free structure
    free(device);
}

static int make_event_map (struct instance *o, int added, const char *ifname, const char *bus, BStringMap *out_map)
{
    // init map
    BStringMap map;
    BStringMap_Init(&map);
    
    // set type
    if (!BStringMap_Set(&map, "event_type", (added ? "added" : "removed"))) {
        ModuleLog(o->i, BLOG_ERROR, "BStringMap_Set failed");
        goto fail1;
    }
    
    // set ifname
    if (!BStringMap_Set(&map, "devname", ifname)) {
        ModuleLog(o->i, BLOG_ERROR, "BStringMap_Set failed");
        goto fail1;
    }
    
    // set bus
    if (!BStringMap_Set(&map, "bus", bus)) {
        ModuleLog(o->i, BLOG_ERROR, "BStringMap_Set failed");
        goto fail1;
    }
    
    *out_map = map;
    return 1;
    
fail1:
    BStringMap_Free(&map);
    return 0;
}

static void queue_event (struct instance *o, BStringMap map)
{
    // pass event to template
    int was_empty;
    event_template_queue(&o->templ, map, &was_empty);
    
    // if event queue was empty, stop receiving udev events
    if (was_empty) {
        NCDUdevClient_Pause(&o->client);
    }
}

static void add_device (struct instance *o, const char *ifname, const char *devpath, uintmax_t ifindex, const char *bus)
{
    ASSERT(!find_device_by_ifname(o, ifname))
    ASSERT(!find_device_by_devpath(o, devpath))
    
    // allocate structure
    struct device *device = malloc(sizeof(*device));
    if (!device) {
        ModuleLog(o->i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // init ifname
    if (!(device->ifname = strdup(ifname))) {
        ModuleLog(o->i, BLOG_ERROR, "strdup failed");
        goto fail1;
    }
    
    // init devpath
    if (!(device->devpath = strdup(devpath))) {
        ModuleLog(o->i, BLOG_ERROR, "strdup failed");
        goto fail2;
    }
    
    // set ifindex
    device->ifindex = ifindex;
    
    // init removed map
    if (!make_event_map(o, 0, ifname, bus, &device->removed_map)) {
        ModuleLog(o->i, BLOG_ERROR, "make_event_map failed");
        goto fail3;
    }
    
    // init added map
    BStringMap added_map;
    if (!make_event_map(o, 1, ifname, bus, &added_map)) {
        ModuleLog(o->i, BLOG_ERROR, "make_event_map failed");
        goto fail4;
    }
    
    // insert to devices list
    LinkedList1_Append(&o->devices_list, &device->devices_list_node);
    
    // queue event
    queue_event(o, added_map);
    
    return;
    
fail4:
    BStringMap_Free(&device->removed_map);
fail3:
    free(device->devpath);
fail2:
    free(device->ifname);
fail1:
    free(device);
fail0:
    ModuleLog(o->i, BLOG_ERROR, "failed to add device %s", ifname);
}

static void remove_device (struct instance *o, struct device *device)
{
    queue_event(o, device->removed_map);
    free_device(o, device, 0);
}

static void next_event (struct instance *o)
{
    ASSERT(event_template_is_enabled(&o->templ))
    
    // order template to finish the current event
    int is_empty;
    event_template_dequeue(&o->templ, &is_empty);
    
    // if template has no events, continue udev events
    if (is_empty) {
        NCDUdevClient_Continue(&o->client);
    }
}

static void make_bus (struct instance *o, const char *devpath, const BStringMap *map, char *out_bus, size_t bus_avail)
{
    regmatch_t pmatch[3];
    
    const char *type;
    const char *id;
    size_t id_len;
    
    if (!regexec(&o->usb_reg, devpath, 3, pmatch, 0)) {
        type = "usb";
        id = devpath + pmatch[2].rm_so;
        id_len = pmatch[2].rm_eo - pmatch[2].rm_so;
    }
    else if (!regexec(&o->pci_reg, devpath, 3, pmatch, 0)) {
        type = "pci";
        id = devpath + pmatch[1].rm_so;
        id_len = pmatch[1].rm_eo - pmatch[1].rm_so;
    } else {
        goto fail;
    }
    
    size_t type_len = strlen(type);
    bsize_t bus_len = bsize_add(bsize_fromsize(type_len), bsize_add(bsize_fromint(1), bsize_add(bsize_fromsize(id_len), bsize_fromint(1))));
    if (bus_len.is_overflow || bus_len.value > bus_avail) {
        goto fail;
    }
    
    memcpy(out_bus, type, type_len);
    out_bus[type_len] = ':';
    memcpy(out_bus + type_len + 1, id, id_len);
    out_bus[type_len + 1 + id_len] = '\0';
    return;
    
fail:
    snprintf(out_bus, bus_avail, "%s", "unknown");
}

static void client_handler (struct instance *o, char *devpath, int have_map, BStringMap map)
{
    // lookup existing device with this devpath
    struct device *ex_device = find_device_by_devpath(o, devpath);
    // lookup cache entry
    const BStringMap *cache_map = NCDUdevManager_Query(o->i->params->iparams->umanager, devpath);
    
    if (!cache_map) {
        if (ex_device) {
            remove_device(o, ex_device);
        }
        goto out;
    }
    
    int match_res = regexec(&o->reg, devpath, 0, NULL, 0);
    const char *interface = BStringMap_Get(cache_map, "INTERFACE");
    const char *ifindex_str = BStringMap_Get(cache_map, "IFINDEX");
    
    uintmax_t ifindex;
    if (!(!match_res && interface && ifindex_str && parse_unsigned_integer(MemRef_MakeCstr(ifindex_str), &ifindex))) {
        if (ex_device) {
            remove_device(o, ex_device);
        }
        goto out;
    }
    
    if (ex_device && (strcmp(ex_device->ifname, interface) || ex_device->ifindex != ifindex)) {
        remove_device(o, ex_device);
        ex_device = NULL;
    }
    
    if (!ex_device) {
        struct device *ex_ifname_device = find_device_by_ifname(o, interface);
        if (ex_ifname_device) {
            remove_device(o, ex_ifname_device);
        }
        
        char bus[128];
        make_bus(o, devpath, cache_map, bus, sizeof(bus));
        
        add_device(o, interface, devpath, ifindex, bus);
    }
    
out:
    free(devpath);
    if (have_map) {
        BStringMap_Free(&map);
    }
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // init client
    NCDUdevClient_Init(&o->client, o->i->params->iparams->umanager, o, (NCDUdevClient_handler)client_handler);
    
    // init devices list
    LinkedList1_Init(&o->devices_list);
    
    // compile regex's
    if (regcomp(&o->reg, DEVPATH_REGEX, REG_EXTENDED)) {
        ModuleLog(o->i, BLOG_ERROR, "regcomp failed");
        goto fail1;
    }
    if (regcomp(&o->usb_reg, DEVPATH_USB_REGEX, REG_EXTENDED)) {
        ModuleLog(o->i, BLOG_ERROR, "regcomp failed");
        goto fail2;
    }
    if (regcomp(&o->pci_reg, DEVPATH_PCI_REGEX, REG_EXTENDED)) {
        ModuleLog(o->i, BLOG_ERROR, "regcomp failed");
        goto fail3;
    }
    
    event_template_new(&o->templ, o->i, BLOG_CURRENT_CHANNEL, 3, o, (event_template_func_free)templ_func_free);
    return;
    
fail3:
    regfree(&o->usb_reg);
fail2:
    regfree(&o->reg);
fail1:
    NCDUdevClient_Free(&o->client);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void templ_func_free (struct instance *o, int is_error)
{
    // free devices
    LinkedList1Node *list_node;
    while (list_node = LinkedList1_GetFirst(&o->devices_list)) {
        struct device *device = UPPER_OBJECT(list_node, struct device, devices_list_node);
        free_device(o, device, 1);
    }
    
    // free regex's
    regfree(&o->pci_reg);
    regfree(&o->usb_reg);
    regfree(&o->reg);
    
    // free client
    NCDUdevClient_Free(&o->client);
    
    if (is_error) {
        NCDModuleInst_Backend_DeadError(o->i);
    } else {
        NCDModuleInst_Backend_Dead(o->i);
    }
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    event_template_die(&o->templ);
}

static int func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    return event_template_getvar(&o->templ, name, mem, out);
}

static void nextevent_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get method object
    struct instance *mo = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // make sure we are currently reporting an event
    if (!event_template_is_enabled(&mo->templ)) {
        ModuleLog(i, BLOG_ERROR, "not reporting an event");
        goto fail0;
    }
    
    // signal up.
    // Do it before finishing the event so our process does not advance any further if
    // we would be killed the event provider going down.
    NCDModuleInst_Backend_Up(i);
    
    // wait for next event
    next_event(mo);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "net.watch_interfaces",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.watch_interfaces::nextevent",
        .func_new2 = nextevent_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_net_watch_interfaces = {
    .modules = modules
};
