/**
 * @file sys_watch_usb.c
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
 * USB device watcher.
 * 
 * Synopsis: sys.watch_usb()
 * Description: reports USB device events. Transitions up when an event is detected, and
 *   goes down waiting for the next event when ->nextevent() is called.
 *   On startup, "added" events are reported for existing USB devices.
 * 
 * Variables:
 *   string event_type - what happened with the USB device: "added" or "removed"
 *   string devname - device node path, e.g. /dev/bus/usb/XXX/YYY
 *   string vendor_id - vendor ID, e.g. 046d
 *   string model_id - model ID, e.g. c03e
 *   
 * Synopsis: sys.watch_usb::nextevent()
 * Description: makes the watch_usb module transition down in order to report the next event.
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <misc/debug.h>
#include <misc/offset.h>
#include <misc/parse_number.h>
#include <structure/LinkedList1.h>
#include <udevmonitor/NCDUdevManager.h>
#include <ncd/modules/event_template.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_sys_watch_usb.h>

struct device {
    char *devname;
    char *devpath;
    uint16_t vendor_id;
    uint16_t model_id;
    BStringMap removed_map;
    LinkedList1Node devices_list_node;
};

struct instance {
    NCDModuleInst *i;
    NCDUdevClient client;
    LinkedList1 devices_list;
    event_template templ;
};

static void templ_func_free (struct instance *o, int is_error);

static struct device * find_device_by_devname (struct instance *o, const char *devname)
{
    for (LinkedList1Node *list_node = LinkedList1_GetFirst(&o->devices_list); list_node; list_node = LinkedList1Node_Next(list_node)) {
        struct device *device = UPPER_OBJECT(list_node, struct device, devices_list_node);
        if (!strcmp(device->devname, devname)) {
            return device;
        }
    }
    
    return NULL;
}

static struct device * find_device_by_devpath (struct instance *o, const char *devpath)
{
    for (LinkedList1Node *list_node = LinkedList1_GetFirst(&o->devices_list); list_node; list_node = LinkedList1Node_Next(list_node)) {
        struct device *device = UPPER_OBJECT(list_node, struct device, devices_list_node);
        if (!strcmp(device->devpath, devpath)) {
            return device;
        }
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
    
    // free devname
    free(device->devname);
    
    // free structure
    free(device);
}

static int make_event_map (struct instance *o, int added, const char *devname, uint16_t vendor_id, uint16_t model_id, BStringMap *out_map)
{
    // init map
    BStringMap map;
    BStringMap_Init(&map);
    
    // set type
    if (!BStringMap_Set(&map, "event_type", (added ? "added" : "removed"))) {
        ModuleLog(o->i, BLOG_ERROR, "BStringMap_Set failed");
        goto fail1;
    }
    
    // set devname
    if (!BStringMap_Set(&map, "devname", devname)) {
        ModuleLog(o->i, BLOG_ERROR, "BStringMap_Set failed");
        goto fail1;
    }
    
    // set vendor ID
    char vendor_id_str[5];
    sprintf(vendor_id_str, "%04"PRIx16, vendor_id);
    if (!BStringMap_Set(&map, "vendor_id", vendor_id_str)) {
        ModuleLog(o->i, BLOG_ERROR, "BStringMap_Set failed");
        goto fail1;
    }
    
    // set model ID
    char model_id_str[5];
    sprintf(model_id_str, "%04"PRIx16, model_id);
    if (!BStringMap_Set(&map, "model_id", model_id_str)) {
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

static void add_device (struct instance *o, const char *devname, const char *devpath, uint16_t vendor_id, uint16_t model_id)
{
    ASSERT(!find_device_by_devname(o, devname))
    ASSERT(!find_device_by_devpath(o, devpath))
    
    // allocate structure
    struct device *device = malloc(sizeof(*device));
    if (!device) {
        ModuleLog(o->i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // init devname
    if (!(device->devname = strdup(devname))) {
        ModuleLog(o->i, BLOG_ERROR, "strdup failed");
        goto fail1;
    }
    
    // init devpath
    if (!(device->devpath = strdup(devpath))) {
        ModuleLog(o->i, BLOG_ERROR, "strdup failed");
        goto fail2;
    }
    
    // set vendor and model ID
    device->vendor_id = vendor_id;
    device->model_id = model_id;
    
    // init removed map
    if (!make_event_map(o, 0, devname, vendor_id, model_id, &device->removed_map)) {
        ModuleLog(o->i, BLOG_ERROR, "make_event_map failed");
        goto fail3;
    }
    
    // init added map
    BStringMap added_map;
    if (!make_event_map(o, 1, devname, vendor_id, model_id, &added_map)) {
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
    free(device->devname);
fail1:
    free(device);
fail0:
    ModuleLog(o->i, BLOG_ERROR, "failed to add device %s", devname);
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
    
    const char *subsystem = BStringMap_Get(cache_map, "SUBSYSTEM");
    const char *devname = BStringMap_Get(cache_map, "DEVNAME");
    const char *devtype = BStringMap_Get(cache_map, "DEVTYPE");
    const char *vendor_id_str = BStringMap_Get(cache_map, "ID_VENDOR_ID");
    const char *model_id_str = BStringMap_Get(cache_map, "ID_MODEL_ID");
    
    uintmax_t vendor_id;
    uintmax_t model_id;
    
    if (!(subsystem && !strcmp(subsystem, "usb") &&
          devname &&
          devtype && !strcmp(devtype, "usb_device") &&
          vendor_id_str && parse_unsigned_hex_integer(MemRef_MakeCstr(vendor_id_str), &vendor_id) &&
          model_id_str && parse_unsigned_hex_integer(MemRef_MakeCstr(model_id_str), &model_id)
    )) {
        if (ex_device) {
            remove_device(o, ex_device);
        }
        goto out;
    }
    
    if (ex_device && (
        strcmp(ex_device->devname, devname) ||
        ex_device->vendor_id != vendor_id || ex_device->model_id != model_id
    )) {
        remove_device(o, ex_device);
        ex_device = NULL;
    }
    
    if (!ex_device) {
        struct device *ex_devname_device = find_device_by_devname(o, devname);
        if (ex_devname_device) {
            remove_device(o, ex_devname_device);
        }
        
        add_device(o, devname, devpath, vendor_id, model_id);
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
    
    event_template_new(&o->templ, o->i, BLOG_CURRENT_CHANNEL, 3, o, (event_template_func_free)templ_func_free);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void templ_func_free (struct instance *o, int is_error)
{
    // free devices
    while (!LinkedList1_IsEmpty(&o->devices_list)) {
        struct device *device = UPPER_OBJECT(LinkedList1_GetFirst(&o->devices_list), struct device, devices_list_node);
        free_device(o, device, 1);
    }
    
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
        .type = "sys.watch_usb",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "sys.watch_usb::nextevent",
        .func_new2 = nextevent_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_sys_watch_usb = {
    .modules = modules
};
