/**
 * @file net_dns.c
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
 * DNS servers module.
 * 
 * Synopsis: net.dns(list(string) servers, string priority)
 * Synopsis: net.dns.resolvconf(list({string type, string value}) lines, string priority)
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <misc/offset.h>
#include <misc/bsort.h>
#include <misc/balloc.h>
#include <misc/compare.h>
#include <misc/concat_strings.h>
#include <misc/expstring.h>
#include <misc/ipaddr.h>
#include <structure/LinkedList1.h>
#include <ncd/extra/NCDIfConfig.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_net_dns.h>

struct instance {
    NCDModuleInst *i;
    LinkedList1 entries;
    LinkedList1Node instances_node; // node in instances
};

struct dns_entry {
    LinkedList1Node list_node; // node in instance.entries
    char *line;
    int priority;
};

struct global {
    LinkedList1 instances;
};

static struct dns_entry * add_dns_entry (struct instance *o, const char *type, const char *value, int priority)
{
    // allocate entry
    struct dns_entry *entry = malloc(sizeof(*entry));
    if (!entry) {
        goto fail0;
    }
    
    // generate line
    entry->line = concat_strings(4, type, " ", value, "\n");
    if (!entry->line) {
        goto fail1;
    }
    
    // set info
    entry->priority = priority;
    
    // add to list
    LinkedList1_Append(&o->entries, &entry->list_node);
    
    return entry;
    
fail1:
    free(entry);
fail0:
    return NULL;
}

static void remove_dns_entry (struct instance *o, struct dns_entry *entry)
{
    // remove from list
    LinkedList1_Remove(&o->entries, &entry->list_node);
    
    // free line
    free(entry->line);
    
    // free entry
    free(entry);
}

static void remove_entries (struct instance *o)
{
    LinkedList1Node *n;
    while (n = LinkedList1_GetFirst(&o->entries)) {
        struct dns_entry *e = UPPER_OBJECT(n, struct dns_entry, list_node);
        remove_dns_entry(o, e);
    }
}

static size_t count_entries (struct global *g)
{
    size_t c = 0;
    
    for (LinkedList1Node *n = LinkedList1_GetFirst(&g->instances); n; n = LinkedList1Node_Next(n)) {
        struct instance *o = UPPER_OBJECT(n, struct instance, instances_node);
        for (LinkedList1Node *en = LinkedList1_GetFirst(&o->entries); en; en = LinkedList1Node_Next(en)) {
            c++;
        }
    }
    
    return c;
}

struct dns_sort_entry {
    char *line;
    int priority;
};

static int dns_sort_comparator (const void *v1, const void *v2)
{
    const struct dns_sort_entry *e1 = v1;
    const struct dns_sort_entry *e2 = v2;
    return B_COMPARE(e1->priority, e2->priority);
}

static int set_servers (struct global *g)
{
    int ret = 0;
    
    // count servers
    size_t num_entries = count_entries(g);
    
    // allocate sort array
    struct dns_sort_entry *sort_entries = BAllocArray(num_entries, sizeof(sort_entries[0]));
    if (!sort_entries) {
        goto fail0;
    }
    
    // fill sort array
    num_entries = 0;
    for (LinkedList1Node *n = LinkedList1_GetFirst(&g->instances); n; n = LinkedList1Node_Next(n)) {
        struct instance *o = UPPER_OBJECT(n, struct instance, instances_node);
        for (LinkedList1Node *en = LinkedList1_GetFirst(&o->entries); en; en = LinkedList1Node_Next(en)) {
            struct dns_entry *e = UPPER_OBJECT(en, struct dns_entry, list_node);
            sort_entries[num_entries].line = e->line;
            sort_entries[num_entries].priority= e->priority;
            num_entries++;
        }
    }
    
    // sort by priority
    // use a custom insertion sort instead of qsort() because we want a stable sort
    struct dns_sort_entry temp;
    BInsertionSort(sort_entries, num_entries, sizeof(sort_entries[0]), dns_sort_comparator, &temp);
    
    ExpString estr;
    if (!ExpString_Init(&estr)) {
        goto fail1;
    }
    
    for (size_t i = 0; i < num_entries; i++) {
        if (!ExpString_Append(&estr, sort_entries[i].line)) {
            goto fail2;
        }
    }
    
    // set servers
    if (!NCDIfConfig_set_resolv_conf(ExpString_Get(&estr), ExpString_Length(&estr))) {
        goto fail2;
    }
    
    ret = 1;
    
fail2:
    ExpString_Free(&estr);
fail1:
    BFree(sort_entries);
fail0:
    return ret;
}

static int func_globalinit (struct NCDInterpModuleGroup *group, const struct NCDModuleInst_iparams *params)
{
    // allocate global state structure
    struct global *g = BAlloc(sizeof(*g));
    if (!g) {
        BLog(BLOG_ERROR, "BAlloc failed");
        return 0;
    }
    
    // set group state pointer
    group->group_state = g;
    
    // init instances list
    LinkedList1_Init(&g->instances);
    
    return 1;
}

static void func_globalfree (struct NCDInterpModuleGroup *group)
{
    struct global *g = group->group_state;
    ASSERT(LinkedList1_IsEmpty(&g->instances))
    
    // free global state structure
    BFree(g);
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct global *g = ModuleGlobal(i);
    struct instance *o = vo;
    o->i = i;
    
    // init servers list
    LinkedList1_Init(&o->entries);
    
    // get arguments
    NCDValRef servers_arg;
    NCDValRef priority_arg;
    if (!NCDVal_ListRead(params->args, 2, &servers_arg, &priority_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail1;
    }
    if (!NCDVal_IsList(servers_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail1;
    }
    
    uintmax_t priority;
    if (!ncd_read_uintmax(priority_arg, &priority) || priority > INT_MAX) {
        ModuleLog(o->i, BLOG_ERROR, "wrong priority");
        goto fail1;
    }
    
    // read servers
    size_t count = NCDVal_ListCount(servers_arg);
    for (size_t j = 0; j < count; j++) {
        NCDValRef server_arg = NCDVal_ListGet(servers_arg, j);
        
        if (!NCDVal_IsString(server_arg)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong type");
            goto fail1;
        }
        
        uint32_t addr;
        if (!ipaddr_parse_ipv4_addr(NCDVal_StringMemRef(server_arg), &addr)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong addr");
            goto fail1;
        }
        
        char addr_str[IPADDR_PRINT_MAX];
        ipaddr_print_addr(addr, addr_str);
        
        if (!add_dns_entry(o, "nameserver", addr_str, priority)) {
            ModuleLog(o->i, BLOG_ERROR, "failed to add dns entry");
            goto fail1;
        }
    }
    
    // add to instances
    LinkedList1_Append(&g->instances, &o->instances_node);
    
    // set servers
    if (!set_servers(g)) {
        ModuleLog(o->i, BLOG_ERROR, "failed to set DNS servers");
        goto fail2;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail2:
    LinkedList1_Remove(&g->instances, &o->instances_node);
fail1:
    remove_entries(o);
    NCDModuleInst_Backend_DeadError(i);
}

static void func_new_resolvconf (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct global *g = ModuleGlobal(i);
    struct instance *o = vo;
    o->i = i;
    
    // init servers list
    LinkedList1_Init(&o->entries);
    
    // get arguments
    NCDValRef lines_arg;
    NCDValRef priority_arg;
    if (!NCDVal_ListRead(params->args, 2, &lines_arg, &priority_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail1;
    }
    if (!NCDVal_IsList(lines_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail1;
    }
    
    uintmax_t priority;
    if (!ncd_read_uintmax(priority_arg, &priority) || priority > INT_MAX) {
        ModuleLog(o->i, BLOG_ERROR, "wrong priority");
        goto fail1;
    }
    
    // read lines
    size_t count = NCDVal_ListCount(lines_arg);
    for (size_t j = 0; j < count; j++) {
        int loop_failed = 1;
        
        NCDValRef line = NCDVal_ListGet(lines_arg, j);
        if (!NCDVal_IsList(line) || NCDVal_ListCount(line) != 2) {
            ModuleLog(o->i, BLOG_ERROR, "lines element is not a list with two elements");
            goto loop_fail0;
        }
        
        NCDValRef type = NCDVal_ListGet(line, 0);
        NCDValRef value = NCDVal_ListGet(line, 1);
        if (!NCDVal_IsStringNoNulls(type) || !NCDVal_IsStringNoNulls(value)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong type of type or value");
            goto loop_fail0;
        }
        
        NCDValNullTermString type_nts;
        if (!NCDVal_StringNullTerminate(type, &type_nts)) {
            ModuleLog(o->i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
            goto loop_fail0;
        }
        
        NCDValNullTermString value_nts;
        if (!NCDVal_StringNullTerminate(value, &value_nts)) {
            ModuleLog(o->i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
            goto loop_fail1;
        }
        
        if (!add_dns_entry(o, type_nts.data, value_nts.data, priority)) {
            ModuleLog(o->i, BLOG_ERROR, "failed to add dns entry");
            goto loop_fail2;
        }
        
        loop_failed = 0;
    loop_fail2:
        NCDValNullTermString_Free(&value_nts);
    loop_fail1:
        NCDValNullTermString_Free(&type_nts);
    loop_fail0:
        if (loop_failed) {
            goto fail1;
        }
    }
    
    // add to instances
    LinkedList1_Append(&g->instances, &o->instances_node);
    
    // set servers
    if (!set_servers(g)) {
        ModuleLog(o->i, BLOG_ERROR, "failed to set DNS servers");
        goto fail2;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail2:
    LinkedList1_Remove(&g->instances, &o->instances_node);
fail1:
    remove_entries(o);
    NCDModuleInst_Backend_DeadError(i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    struct global *g = ModuleGlobal(o->i);
    
    // remove from instances
    LinkedList1_Remove(&g->instances, &o->instances_node);
    
    // set servers
    set_servers(g);
    
    // free servers
    remove_entries(o);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static struct NCDModule modules[] = {
    {
        .type = "net.dns",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.dns.resolvconf",
        .func_new2 = func_new_resolvconf,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_net_dns = {
    .func_globalinit = func_globalinit,
    .func_globalfree = func_globalfree,
    .modules = modules
};
