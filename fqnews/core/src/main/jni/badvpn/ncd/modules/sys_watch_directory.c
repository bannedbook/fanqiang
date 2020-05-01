/**
 * @file sys_watch_directory.c
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
 * Directory watcher.
 * 
 * Synopsis: sys.watch_directory(string dir)
 * Description: reports directory entry events. Transitions up when an event is detected, and
 *   goes down waiting for the next event when sys.watch_directory::nextevent() is called.
 *   The directory is first scanned and "added" events are reported for all files.
 * Variables:
 *   string event_type - what happened with the file: "added", "removed" or "changed"
 *   string filename - name of the file in the directory the event refers to
 *   string filepath - "dir/filename"
 * 
 * Synopsis: sys.watch_directory::nextevent()
 * Description: makes the watch_directory module transition down in order to report the next event.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <misc/nonblocking.h>
#include <misc/concat_strings.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_sys_watch_directory.h>

#define MAX_INOTIFY_EVENTS 128

struct instance {
    NCDModuleInst *i;
    NCDValNullTermString dir_nts;
    DIR *dir_handle;
    int inotify_fd;
    BFileDescriptor bfd;
    struct inotify_event events[MAX_INOTIFY_EVENTS];
    int events_count;
    int events_index;
    int processing;
    const char *processing_file;
    const char *processing_type;
};

static void instance_free (struct instance *o, int is_error);

static void next_dir_event (struct instance *o)
{
    ASSERT(!o->processing)
    ASSERT(o->dir_handle)
    
    struct dirent *entry;
    
    do {
        // get next entry
        errno = 0;
        if (!(entry = readdir(o->dir_handle))) {
            if (errno != 0) {
                ModuleLog(o->i, BLOG_ERROR, "readdir failed");
                instance_free(o, 1);
                return;
            }
            
            // close directory
            if (closedir(o->dir_handle) < 0) {
                ModuleLog(o->i, BLOG_ERROR, "closedir failed");
                o->dir_handle = NULL;
                instance_free(o, 1);
                return;
            }
            
            // set no dir handle
            o->dir_handle = NULL;
            
            // start receiving inotify events
            BReactor_SetFileDescriptorEvents(o->i->params->iparams->reactor, &o->bfd, BREACTOR_READ);
            return;
        }
    } while (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."));
    
    // set event
    o->processing_file = entry->d_name;
    o->processing_type = "added";
    o->processing = 1;
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
}

static void assert_inotify_event (struct instance *o)
{
    ASSERT(o->events_index < o->events_count)
    ASSERT(o->events[o->events_index].len % sizeof(o->events[0]) == 0)
    ASSERT(o->events[o->events_index].len / sizeof(o->events[0]) <= o->events_count - (o->events_index + 1))
}

static const char * translate_inotify_event (struct instance *o)
{
    assert_inotify_event(o);
    
    struct inotify_event *event = &o->events[o->events_index];
    
    if (strlen(event->name) > 0) {
        if ((event->mask & (IN_CREATE | IN_MOVED_TO))) {
            return "added";
        }
        if ((event->mask & (IN_DELETE | IN_MOVED_FROM))) {
            return "removed";
        }
        if ((event->mask & IN_MODIFY)) {
            return "changed";
        }
    }
    
    return NULL;
}

static void skip_inotify_event (struct instance *o)
{
    assert_inotify_event(o);
    
    o->events_index += 1 + o->events[o->events_index].len / sizeof(o->events[0]);
}

static void next_inotify_event (struct instance *o)
{
    ASSERT(!o->processing)
    ASSERT(!o->dir_handle)
    
    // skip any bad events
    while (o->events_index < o->events_count && !translate_inotify_event(o)) {
        ModuleLog(o->i, BLOG_ERROR, "unknown inotify event");
        skip_inotify_event(o);
    }
    
    if (o->events_index == o->events_count) {
        // wait for more events
        BReactor_SetFileDescriptorEvents(o->i->params->iparams->reactor, &o->bfd, BREACTOR_READ);
        return;
    }
    
    // set event
    o->processing_file = o->events[o->events_index].name;
    o->processing_type = translate_inotify_event(o);
    o->processing = 1;
    
    // consume this event
    skip_inotify_event(o);
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
}

static void inotify_fd_handler (struct instance *o, int events)
{
    if (o->processing) {
        ModuleLog(o->i, BLOG_ERROR, "file descriptor error");
        instance_free(o, 1);
        return;
    }
    
    ASSERT(!o->dir_handle)
    
    int res = read(o->inotify_fd, o->events, sizeof(o->events));
    if (res < 0) {
        ModuleLog(o->i, BLOG_ERROR, "read failed");
        instance_free(o, 1);
        return;
    }
    
    // stop waiting for inotify events
    BReactor_SetFileDescriptorEvents(o->i->params->iparams->reactor, &o->bfd, 0);
    
    ASSERT(res <= sizeof(o->events))
    ASSERT(res % sizeof(o->events[0]) == 0)
    
    // setup buffer state
    o->events_count = res / sizeof(o->events[0]);
    o->events_index = 0;
    
    // process inotify events
    next_inotify_event(o);
}

static void next_event (struct instance *o)
{
    ASSERT(o->processing)
    
    // set not processing
    o->processing = 0;
    
    // signal down
    NCDModuleInst_Backend_Down(o->i);
    
    if (o->dir_handle) {
        next_dir_event(o);
        return;
    } else {
        next_inotify_event(o);
        return;
    }
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef dir_arg;
    if (!NCDVal_ListRead(params->args, 1, &dir_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(dir_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // null terminate dir
    if (!NCDVal_StringNullTerminate(dir_arg, &o->dir_nts)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    // open inotify
    if ((o->inotify_fd = inotify_init()) < 0) {
        ModuleLog(o->i, BLOG_ERROR, "inotify_init failed");
        goto fail1;
    }
    
    // add watch
    if (inotify_add_watch(o->inotify_fd, o->dir_nts.data, IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO) < 0) {
        ModuleLog(o->i, BLOG_ERROR, "inotify_add_watch failed");
        goto fail2;
    }
    
    // set non-blocking
    if (!badvpn_set_nonblocking(o->inotify_fd)) {
        ModuleLog(o->i, BLOG_ERROR, "badvpn_set_nonblocking failed");
        goto fail2;
    }
    
    // init BFileDescriptor
    BFileDescriptor_Init(&o->bfd, o->inotify_fd, (BFileDescriptor_handler)inotify_fd_handler, o);
    if (!BReactor_AddFileDescriptor(o->i->params->iparams->reactor, &o->bfd)) {
        ModuleLog(o->i, BLOG_ERROR, "BReactor_AddFileDescriptor failed");
        goto fail2;
    }
    
    // open directory
    if (!(o->dir_handle = opendir(o->dir_nts.data))) {
        ModuleLog(o->i, BLOG_ERROR, "opendir failed");
        goto fail3;
    }
    
    // set not processing
    o->processing = 0;
    
    // read first directory entry
    next_dir_event(o);
    return;
    
fail3:
    BReactor_RemoveFileDescriptor(o->i->params->iparams->reactor, &o->bfd);
fail2:
    if (close(o->inotify_fd) < 0) {
        ModuleLog(o->i, BLOG_ERROR, "close failed");
    }
fail1:
    NCDValNullTermString_Free(&o->dir_nts);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

void instance_free (struct instance *o, int is_error)
{
    // close directory
    if (o->dir_handle) {
        if (closedir(o->dir_handle) < 0) {
            ModuleLog(o->i, BLOG_ERROR, "closedir failed");
        }
    }
    
    // free BFileDescriptor
    BReactor_RemoveFileDescriptor(o->i->params->iparams->reactor, &o->bfd);
    
    // close inotify
    if (close(o->inotify_fd) < 0) {
        ModuleLog(o->i, BLOG_ERROR, "close failed");
    }
    
    // free dir nts
    NCDValNullTermString_Free(&o->dir_nts);
    
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

static int func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    ASSERT(o->processing)
    
    if (!strcmp(name, "event_type")) {
        *out = NCDVal_NewString(mem, o->processing_type);
        return 1;
    }
    
    if (!strcmp(name, "filename")) {
        *out = NCDVal_NewString(mem, o->processing_file);
        return 1;
    }
    
    if (!strcmp(name, "filepath")) {
        char *str = concat_strings(3, o->dir_nts.data, "/", o->processing_file);
        if (!str) {
            ModuleLog(o->i, BLOG_ERROR, "concat_strings failed");
            goto fail;
        }
        
        *out = NCDVal_NewString(mem, str);
        
        free(str);
        return 1;
    }
    
    return 0;
    
fail:
    *out = NCDVal_NewInvalid();
    return 1;
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
    next_event(mo);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "sys.watch_directory",
        .func_new2 = func_new,
        .func_die = func_die,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "sys.watch_directory::nextevent",
        .func_new2 = nextevent_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_sys_watch_directory = {
    .modules = modules
};
