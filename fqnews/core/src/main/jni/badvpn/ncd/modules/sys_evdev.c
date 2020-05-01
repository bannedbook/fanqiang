/**
 * @file sys_evdev.c
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
 * Linux event device module.
 * 
 * Synopsis: sys.evdev(string device)
 * Description: reports input events from a Linux event device. Transitions up when an event is
 *   detected, and goes down waiting for the next event when sys.evdev::nextevent() is called.
 * Variables:
 *   string type - symbolic event type (e.g. EV_KEY, EV_REL, EV_ABS), corresponding to
 *     (struct input_event).type, or "unknown"
 *   string value - event value (signed integer), equal to (struct input_event).value
 *   string code_numeric - numeric event code (unsigned integer), equal to
 *     (struct input_event).code
 *   string code - symbolic event code (e.g. KEY_ESC. KEY_1, KEY_2, BTN_LEFT), corrresponding
 *     to (struct input_event).code, or "unknown"
 * 
 * Synopsis: sys.evdev::nextevent()
 * Description: makes the evdev module transition down in order to report the next event.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include <misc/nonblocking.h>
#include <misc/debug.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_sys_evdev.h>

#include "linux_input_names.h"

struct instance {
    NCDModuleInst *i;
    int evdev_fd;
    BFileDescriptor bfd;
    int processing;
    struct input_event event;
};

static void instance_free (struct instance *o, int is_error);

enum {STRING_VALUE, STRING_CODE_NUMERIC, STRING_CODE};

static const char *strings[] = {
    "value", "code_numeric", "code", NULL
};

#define MAKE_LOOKUP_FUNC(_name_) \
static const char * evdev_##_name_##_to_str (uint16_t type) \
{ \
    if (type >= (sizeof(_name_##_names) / sizeof(_name_##_names[0])) || !_name_##_names[type]) { \
        return "unknown"; \
    } \
    return _name_##_names[type]; \
}

MAKE_LOOKUP_FUNC(type)
MAKE_LOOKUP_FUNC(syn)
MAKE_LOOKUP_FUNC(key)
MAKE_LOOKUP_FUNC(rel)
MAKE_LOOKUP_FUNC(abs)
MAKE_LOOKUP_FUNC(sw)
MAKE_LOOKUP_FUNC(msc)
MAKE_LOOKUP_FUNC(led)
MAKE_LOOKUP_FUNC(rep)
MAKE_LOOKUP_FUNC(snd)
MAKE_LOOKUP_FUNC(ffstatus)

static void device_handler (struct instance *o, int events)
{
    if (o->processing) {
        ModuleLog(o->i, BLOG_ERROR, "device error");
        instance_free(o, 1);
        return;
    }
    
    int res = read(o->evdev_fd, &o->event, sizeof(o->event));
    if (res < 0) {
        ModuleLog(o->i, BLOG_ERROR, "read failed");
        instance_free(o, 1);
        return;
    }
    if (res != sizeof(o->event)) {
        ModuleLog(o->i, BLOG_ERROR, "read wrong");
        instance_free(o, 1);
        return;
    }
    
    // stop reading
    BReactor_SetFileDescriptorEvents(o->i->params->iparams->reactor, &o->bfd, 0);
    
    // set processing
    o->processing = 1;
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
}

static void device_nextevent (struct instance *o)
{
    ASSERT(o->processing)
    
    // start reading
    BReactor_SetFileDescriptorEvents(o->i->params->iparams->reactor, &o->bfd, BREACTOR_READ);
    
    // set not processing
    o->processing = 0;
    
    // signal down
    NCDModuleInst_Backend_Down(o->i);
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef device_arg;
    if (!NCDVal_ListRead(params->args, 1, &device_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(device_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // null terminate device
    NCDValNullTermString device_nts;
    if (!NCDVal_StringNullTerminate(device_arg, &device_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    // open device
    o->evdev_fd = open(device_nts.data, O_RDONLY);
    NCDValNullTermString_Free(&device_nts);
    if (o->evdev_fd < 0) {
        ModuleLog(o->i, BLOG_ERROR, "open failed");
        goto fail0;
    }
    
    // set non-blocking
    if (!badvpn_set_nonblocking(o->evdev_fd)) {
        ModuleLog(o->i, BLOG_ERROR, "badvpn_set_nonblocking failed");
        goto fail1;
    }
    
    // init BFileDescriptor
    BFileDescriptor_Init(&o->bfd, o->evdev_fd, (BFileDescriptor_handler)device_handler, o);
    if (!BReactor_AddFileDescriptor(o->i->params->iparams->reactor, &o->bfd)) {
        ModuleLog(o->i, BLOG_ERROR, "BReactor_AddFileDescriptor failed");
        goto fail1;
    }
    BReactor_SetFileDescriptorEvents(o->i->params->iparams->reactor, &o->bfd, BREACTOR_READ);
    
    // set not processing
    o->processing = 0;
    return;
    
fail1:
    if (close(o->evdev_fd) < 0) {
        ModuleLog(o->i, BLOG_ERROR, "close failed");
    }
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

void instance_free (struct instance *o, int is_error)
{
    // free BFileDescriptor
    BReactor_RemoveFileDescriptor(o->i->params->iparams->reactor, &o->bfd);
    
    // close device.
    // Ignore close error which happens if the device is removed.
    if (close(o->evdev_fd) < 0) {
        ModuleLog(o->i, BLOG_ERROR, "close failed");
    }
    
    if (is_error) {
        NCDModuleInst_Backend_DeadError(o->i);
    } else {
        NCDModuleInst_Backend_Dead(o->i);
    }
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    instance_free(o, 0);
}

static int func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    ASSERT(o->processing)
    
    if (name == NCD_STRING_TYPE) {
        *out = NCDVal_NewString(mem, evdev_type_to_str(o->event.type));
        return 1;
    }
    
    if (name == ModuleString(o->i, STRING_VALUE)) {
        char str[50];
        snprintf(str, sizeof(str), "%"PRIi32, o->event.value);
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    if (name == ModuleString(o->i, STRING_CODE_NUMERIC)) {
        *out = ncd_make_uintmax(mem, o->event.code);
        return 1;
    }
    
    if (name == ModuleString(o->i, STRING_CODE)) {
        const char *str = "unknown";
        
        #define MAKE_CASE(_evname_, _name_) \
            case _evname_: \
                str = evdev_##_name_##_to_str(o->event.code); \
                break;
        
        switch (o->event.type) {
            #ifdef EV_KEY
            MAKE_CASE(EV_KEY, key)
            #endif
            #ifdef EV_SYN
            MAKE_CASE(EV_SYN, syn)
            #endif
            #ifdef EV_REL
            MAKE_CASE(EV_REL, rel)
            #endif
            #ifdef EV_ABS
            MAKE_CASE(EV_ABS, abs)
            #endif
            #ifdef EV_SW
            MAKE_CASE(EV_SW, sw)
            #endif
            #ifdef EV_MSC
            MAKE_CASE(EV_MSC, msc)
            #endif
            #ifdef EV_LED
            MAKE_CASE(EV_LED, led)
            #endif
            #ifdef EV_REP
            MAKE_CASE(EV_REP, rep)
            #endif
            #ifdef EV_SND
            MAKE_CASE(EV_SND, snd)
            #endif
            #ifdef EV_FF_STATUS
            MAKE_CASE(EV_FF_STATUS, ffstatus)
            #endif
        }
        
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    return 0;
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
    if (!mo->processing) {
        ModuleLog(i, BLOG_ERROR, "not reporting an event");
        goto fail0;
    }
    
    // signal up.
    // Do it before finishing the event so our process does not advance any further if
    // we would be killed the event provider going down.
    NCDModuleInst_Backend_Up(i);
    
    // wait for next event
    device_nextevent(mo);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "sys.evdev",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar2 = func_getvar2,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "sys.evdev::nextevent",
        .func_new2 = nextevent_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_sys_evdev = {
    .modules = modules,
    .strings = strings
};
