/**
 * @file sys_watch_input.c
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
 * Input device watcher.
 * 
 * Synopsis: sys.watch_input(string devnode_type)
 * Description: reports input device events. Transitions up when an event is detected, and
 *   goes down waiting for the next event when sys.watch_input::nextevent() is called.
 *   On startup, "added" events are reported for existing input devices.
 * Arguments:
 *   string devnode_type - device node type, for example "event", "mouse" or "js".
 * Variables:
 *   string event_type - what happened with the input device: "added" or "removed"
 *   string devname - device node path
 *   string device_type - input device type, "tablet", "joystick", "touchscreen", "mouse",
 *     "touchpad", "key", "keyboard" or "unknown"
 * 
 * Synopsis: sys.watch_input::nextevent()
 * Description: makes the watch_input module transition down in order to report the next event.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <misc/debug.h>
#include <misc/offset.h>
#include <structure/LinkedList1.h>
#include <udevmonitor/NCDUdevManager.h>
#include <ncd/modules/event_template.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_sys_watch_input.h>

struct device {
    char *devname;
    char *devpath;
    BStringMap removed_map;
    LinkedList1Node devices_list_node;
};

struct instance {
    NCDModuleInst *i;
    MemRef devnode_type;
    NCDUdevClient client;
    LinkedList1 devices_list;
    event_template templ;
};

static void templ_func_free (struct instance *o, int is_error);

static struct device * find_device_by_devname (struct instance *o, const char *devname)
{
    LinkedList1Node *list_node = LinkedList1_GetFirst(&o->devices_list);
    while (list_node) {
        struct device *device = UPPER_OBJECT(list_node, struct device, devices_list_node);
        if (!strcmp(device->devname, devname)) {
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
    
    // free devname
    free(device->devname);
    
    // free structure
    free(device);
}

static int make_event_map (struct instance *o, int added, const char *devname, const char *device_type, BStringMap *out_map)
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
    
    // set device type
    if (!BStringMap_Set(&map, "device_type", device_type)) {
        ModuleLog(o->i, BLOG_ERROR, "BStringMap_Set failed");
        goto fail1;
    }
    
    *out_map = map;
    return 1;
    
fail1:
    BStringMap_Free(&map);
    return 0;
}

static int devname_is_type (const char *devname, MemRef devname_type)
{
    // skip digits at the end
    size_t i;
    for (i = strlen(devname); i > 0; i--) {
        if (!isdigit(devname[i - 1])) {
            break;
        }
    }
    
    // check if devname_type precedes skipped digits
    for (size_t j = devname_type.len; j > 0; j--) {
        if (!(i > 0 && devname[i - 1] == devname_type.ptr[j - 1])) {
            return 0;
        }
        i--;
    }
    
    // check if slash precedes devname_type
    if (!(i > 0 && devname[i - 1] == '/')) {
        return 0;
    }
    
    return 1;
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

static void add_device (struct instance *o, const char *devname, const char *devpath, const char *device_type)
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
    
    // init removed map
    if (!make_event_map(o, 0, devname, device_type, &device->removed_map)) {
        ModuleLog(o->i, BLOG_ERROR, "make_event_map failed");
        goto fail3;
    }
    
    // init added map
    BStringMap added_map;
    if (!make_event_map(o, 1, devname, device_type, &added_map)) {
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
    
    if (!(subsystem && !strcmp(subsystem, "input") && devname && devname_is_type(devname, o->devnode_type))) {
        if (ex_device) {
            remove_device(o, ex_device);
        }
        goto out;
    }
    
    if (ex_device && strcmp(ex_device->devname, devname)) {
        remove_device(o, ex_device);
        ex_device = NULL;
    }
    
    if (!ex_device) {
        struct device *ex_devname_device = find_device_by_devname(o, devname);
        if (ex_devname_device) {
            remove_device(o, ex_devname_device);
        }
        
        // determine type
        const char *device_type = "unknown";
        if (BStringMap_Get(cache_map, "ID_INPUT_TABLET")) {
            device_type = "tablet";
        }
        else if (BStringMap_Get(cache_map, "ID_INPUT_JOYSTICK")) {
            device_type = "joystick";
        }
        else if (BStringMap_Get(cache_map, "ID_INPUT_TOUCHSCREEN")) {
            device_type = "touchscreen";
        }
        else if (BStringMap_Get(cache_map, "ID_INPUT_MOUSE")) {
            device_type = "mouse";
        }
        else if (BStringMap_Get(cache_map, "ID_INPUT_TOUCHPAD")) {
            device_type = "touchpad";
        }
        else if (BStringMap_Get(cache_map, "ID_INPUT_KEY")) {
            device_type = "key";
        }
        else if (BStringMap_Get(cache_map, "ID_INPUT_KEYBOARD")) {
            device_type = "keyboard";
        }
        
        add_device(o, devname, devpath, device_type);
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
    NCDValRef devnode_type_arg;
    if (!NCDVal_ListRead(params->args, 1, &devnode_type_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(devnode_type_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    o->devnode_type = NCDVal_StringMemRef(devnode_type_arg);
    
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
    LinkedList1Node *list_node;
    while (list_node = LinkedList1_GetFirst(&o->devices_list)) {
        struct device *device = UPPER_OBJECT(list_node, struct device, devices_list_node);
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
        .type = "sys.watch_input",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "sys.watch_input::nextevent",
        .func_new2 = nextevent_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_sys_watch_input = {
    .modules = modules
};
