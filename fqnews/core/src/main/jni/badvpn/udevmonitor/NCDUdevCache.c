/**
 * @file NCDUdevCache.c
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
#include <misc/string_begins_with.h>
#include <misc/concat_strings.h>
#include <misc/compare.h>
#include <base/BLog.h>

#include <udevmonitor/NCDUdevCache.h>

#include <generated/blog_channel_NCDUdevCache.h>

static int string_comparator (void *unused, const char **str1, const char **str2)
{
    int c = strcmp(*str1, *str2);
    return B_COMPARE(c, 0);
}

static void free_device (NCDUdevCache *o, struct NCDUdevCache_device *device)
{
    if (device->is_cleaned) {
        // remove from cleaned devices list
        LinkedList1_Remove(&o->cleaned_devices_list, &device->cleaned_devices_list_node);
    } else {
        // remove from devices tree
        BAVL_Remove(&o->devices_tree, &device->devices_tree_node);
    }
    
    // free map
    BStringMap_Free(&device->map);
    
    // free structure
    free(device);
}

static struct NCDUdevCache_device * lookup_device (NCDUdevCache *o, const char *devpath)
{
    BAVLNode *tree_node = BAVL_LookupExact(&o->devices_tree, &devpath);
    if (!tree_node) {
        return NULL;
    }
    struct NCDUdevCache_device *device = UPPER_OBJECT(tree_node, struct NCDUdevCache_device, devices_tree_node);
    ASSERT(!device->is_cleaned)
    
    return device;
}

static void rename_devices (NCDUdevCache *o, const char *prefix, const char *new_prefix)
{
    ASSERT(strlen(prefix) > 0)
    
    size_t prefix_len = strlen(prefix);
    
    // lookup prefix
    BAVLNode *tree_node = BAVL_Lookup(&o->devices_tree, &prefix);
    if (!tree_node) {
        return;
    }
    struct NCDUdevCache_device *device = UPPER_OBJECT(tree_node, struct NCDUdevCache_device, devices_tree_node);
    ASSERT(!device->is_cleaned)
    
    // if the result does not begin with prefix, we might gave gotten the device before all
    // devices beginning with prefix, so skip it
    if (!string_begins_with(device->devpath, prefix)) {
        tree_node = BAVL_GetNext(&o->devices_tree, tree_node);
    }
    
    while (tree_node) {
        // get next node (must be here because we rename this device)
        BAVLNode *next_tree_node = BAVL_GetNext(&o->devices_tree, tree_node);
        
        // get device
        device = UPPER_OBJECT(tree_node, struct NCDUdevCache_device, devices_tree_node);
        ASSERT(!device->is_cleaned)
        
        // if it doesn't begin with prefix, we're done
        if (!string_begins_with(device->devpath, prefix)) {
            break;
        }
        
        // build new devpath
        char *new_devpath = concat_strings(2, new_prefix, device->devpath + prefix_len);
        if (!new_devpath) {
            BLog(BLOG_ERROR, "concat_strings failed");
            goto fail_loop0;
        }
        
        // make sure the new name does not exist
        if (BAVL_LookupExact(&o->devices_tree, &new_devpath)) {
            BLog(BLOG_ERROR, "rename destination already exists");
            goto fail_loop1;
        }
        
        BLog(BLOG_DEBUG, "rename %s -> %s", device->devpath, new_devpath);
        
        // remove from tree
        BAVL_Remove(&o->devices_tree, &device->devices_tree_node);
        
        // update devpath in map
        if (!BStringMap_Set(&device->map, "DEVPATH", new_devpath)) {
            BLog(BLOG_ERROR, "BStringMap_Set failed");
            ASSERT_EXECUTE(BAVL_Insert(&o->devices_tree, &device->devices_tree_node, NULL))
            goto fail_loop1;
        }
        
        // update devpath pointer
        device->devpath = BStringMap_Get(&device->map, "DEVPATH");
        ASSERT(device->devpath)
        
        // insert to tree
        ASSERT_EXECUTE(BAVL_Insert(&o->devices_tree, &device->devices_tree_node, NULL))
        
    fail_loop1:
        free(new_devpath);
    fail_loop0:
        tree_node = next_tree_node;
    }
}

static int add_device (NCDUdevCache *o, BStringMap map)
{
    ASSERT(BStringMap_Get(&map, "DEVPATH"))
    
    // alloc structure
    struct NCDUdevCache_device *device = malloc(sizeof(*device));
    if (!device) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // init map
    device->map = map;
    
    // set device path
    device->devpath = BStringMap_Get(&device->map, "DEVPATH");
    
    // insert to devices tree
    BAVLNode *ex_node;
    if (!BAVL_Insert(&o->devices_tree, &device->devices_tree_node, &ex_node)) {
        BLog(BLOG_DEBUG, "update %s", device->devpath);
        
        // get existing device
        struct NCDUdevCache_device *ex_device = UPPER_OBJECT(ex_node, struct NCDUdevCache_device, devices_tree_node);
        ASSERT(!ex_device->is_cleaned)
        
        // remove exiting device
        free_device(o, ex_device);
        
        // insert
        ASSERT_EXECUTE(BAVL_Insert(&o->devices_tree, &device->devices_tree_node, NULL))
    } else {
        BLog(BLOG_DEBUG, "add %s", device->devpath);
    }
    
    // set not cleaned
    device->is_cleaned = 0;
    
    // set refreshed
    device->is_refreshed = 1;
    
    return 1;
    
fail0:
    return 0;
}

void NCDUdevCache_Init (NCDUdevCache *o)
{
    // init devices tree
    BAVL_Init(&o->devices_tree, OFFSET_DIFF(struct NCDUdevCache_device, devpath, devices_tree_node), (BAVL_comparator)string_comparator, NULL);
    
    // init cleaned devices list
    LinkedList1_Init(&o->cleaned_devices_list);
    
    DebugObject_Init(&o->d_obj);
}

void NCDUdevCache_Free (NCDUdevCache *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free cleaned devices
    LinkedList1Node *list_node;
    while (list_node = LinkedList1_GetFirst(&o->cleaned_devices_list)) {
        struct NCDUdevCache_device *device = UPPER_OBJECT(list_node, struct NCDUdevCache_device, cleaned_devices_list_node);
        ASSERT(device->is_cleaned)
        free_device(o, device);
    }
    
    // free devices
    BAVLNode *tree_node;
    while (tree_node = BAVL_GetFirst(&o->devices_tree)) {
        struct NCDUdevCache_device *device = UPPER_OBJECT(tree_node, struct NCDUdevCache_device, devices_tree_node);
        ASSERT(!device->is_cleaned)
        free_device(o, device);
    }
}

const BStringMap * NCDUdevCache_Query (NCDUdevCache *o, const char *devpath)
{
    DebugObject_Access(&o->d_obj);
    
    // lookup device
    struct NCDUdevCache_device *device = lookup_device(o, devpath);
    if (!device) {
        return NULL;
    }
    
    // return map
    return &device->map;
}

int NCDUdevCache_Event (NCDUdevCache *o, BStringMap map)
{
    DebugObject_Access(&o->d_obj);
    
    // get device path
    const char *devpath = BStringMap_Get(&map, "DEVPATH");
    if (!devpath) {
        BLog(BLOG_ERROR, "missing DEVPATH");
        goto fail;
    }
    
    // get action
    const char *action = BStringMap_Get(&map, "ACTION");
    
    // if this is a remove event, remove device if we have it
    if (action && !strcmp(action, "remove")) {
        // remove existing device
        struct NCDUdevCache_device *device = lookup_device(o, devpath);
        if (device) {
            BLog(BLOG_DEBUG, "remove %s", devpath);
            free_device(o, device);
        } else {
            BLog(BLOG_DEBUG, "remove unknown %s", devpath);
        }
        
        // eat map
        BStringMap_Free(&map);
        
        return 1;
    }
    
    // if this is a move event, remove old device and contaned devices
    if (action && !strcmp(action, "move")) {
        const char *devpath_old = BStringMap_Get(&map, "DEVPATH_OLD");
        if (!devpath_old) {
            goto fail_rename0;
        }
        
        // remove old device
        struct NCDUdevCache_device *old_device = lookup_device(o, devpath_old);
        if (old_device) {
            BLog(BLOG_DEBUG, "remove moved %s", old_device->devpath);
            free_device(o, old_device);
        }
        
        // construct prefix "<devpath_old>/" and new prefix "<devpath>/"
        char *prefix = concat_strings(2, devpath_old, "/");
        if (!prefix) {
            BLog(BLOG_ERROR, "concat_strings failed");
            goto fail_rename0;;
        }
        char *new_prefix = concat_strings(2, devpath, "/");
        if (!new_prefix) {
            BLog(BLOG_ERROR, "concat_strings failed");
            goto fail_rename1;
        }
        
        // rename devices with paths starting with prefix
        rename_devices(o, prefix, new_prefix);
        
        free(new_prefix);
    fail_rename1:
        free(prefix);
    fail_rename0:;
    }
    
    // add device
    if (!add_device(o, map)) {
        BLog(BLOG_DEBUG, "failed to add device %s", devpath);
        goto fail;
    }
    
    return 1;
    
fail:
    return 0;
}

void NCDUdevCache_StartClean (NCDUdevCache *o)
{
    DebugObject_Access(&o->d_obj);
    
    // mark all devices not refreshed
    BAVLNode *tree_node = BAVL_GetFirst(&o->devices_tree);
    while (tree_node) {
        struct NCDUdevCache_device *device = UPPER_OBJECT(tree_node, struct NCDUdevCache_device, devices_tree_node);
        ASSERT(!device->is_cleaned)
        
        // set device not refreshed
        device->is_refreshed = 0;
        
        tree_node = BAVL_GetNext(&o->devices_tree, tree_node);
    }
}

void NCDUdevCache_FinishClean (NCDUdevCache *o)
{
    DebugObject_Access(&o->d_obj);
    
    // move all devices not marked refreshed to the cleaned devices list
    BAVLNode *tree_node = BAVL_GetFirst(&o->devices_tree);
    while (tree_node) {
        BAVLNode *next_tree_node = BAVL_GetNext(&o->devices_tree, tree_node);
        
        struct NCDUdevCache_device *device = UPPER_OBJECT(tree_node, struct NCDUdevCache_device, devices_tree_node);
        ASSERT(!device->is_cleaned)
        
        if (!device->is_refreshed) {
            BLog(BLOG_DEBUG, "clean %s", device->devpath);
            
            // remove from devices tree
            BAVL_Remove(&o->devices_tree, &device->devices_tree_node);
            
            // insert to cleaned devices list
            LinkedList1_Append(&o->cleaned_devices_list, &device->cleaned_devices_list_node);
            
            // set device cleaned
            device->is_cleaned = 1;
        }
        
        tree_node = next_tree_node;
    }
}

int NCDUdevCache_GetCleanedDevice (NCDUdevCache *o, BStringMap *out_map)
{
    DebugObject_Access(&o->d_obj);
    
    // get cleaned device
    LinkedList1Node *list_node = LinkedList1_GetFirst(&o->cleaned_devices_list);
    if (!list_node) {
        return 0;
    }
    struct NCDUdevCache_device *device = UPPER_OBJECT(list_node, struct NCDUdevCache_device, cleaned_devices_list_node);
    ASSERT(device->is_cleaned)
    
    // remove from cleaned devices list
    LinkedList1_Remove(&o->cleaned_devices_list, &device->cleaned_devices_list_node);
    
    // give away map
    *out_map = device->map;
    
    // free structure
    free(device);
    
    return 1;
}

const char * NCDUdevCache_First (NCDUdevCache *o)
{
    DebugObject_Access(&o->d_obj);
    
    BAVLNode *tree_node = BAVL_GetFirst(&o->devices_tree);
    if (!tree_node) {
        return NULL;
    }
    struct NCDUdevCache_device *device = UPPER_OBJECT(tree_node, struct NCDUdevCache_device, devices_tree_node);
    ASSERT(!device->is_cleaned)
    
    return device->devpath;
}

const char * NCDUdevCache_Next (NCDUdevCache *o, const char *key)
{
    ASSERT(BAVL_LookupExact(&o->devices_tree, &key))
    
    BAVLNode *tree_node = BAVL_GetNext(&o->devices_tree, BAVL_LookupExact(&o->devices_tree, &key));
    if (!tree_node) {
        return NULL;
    }
    struct NCDUdevCache_device *device = UPPER_OBJECT(tree_node, struct NCDUdevCache_device, devices_tree_node);
    ASSERT(!device->is_cleaned)
    
    return device->devpath;
}
