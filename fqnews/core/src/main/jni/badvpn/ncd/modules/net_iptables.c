/**
 * @file net_iptables.c
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
 * iptables and ebtables module.
 * 
 * Note that all iptables/ebtables commands (in general) must be issued synchronously, or
 * the kernel may randomly report errors if there is another iptables/ebtables command in
 * progress. To solve this, the NCD process contains a single "iptables lock". All
 * iptables/ebtables commands exposed here go through that lock.
 * In case you wish to call iptables/ebtables directly, the lock is exposed via
 * net.iptables.lock().
 * 
 * The append and insert commands, instead of using the variable-argument form below
 * as documented below, may alternatively be called with a single list argument.
 * 
 * Synopsis:
 *   net.iptables.append(string table, string chain, string arg1  ...)
 * Description:
 *   init:   iptables -t table -A chain arg1 ...
 *   deinit: iptables -t table -D chain arg1 ...
 * 
 * Synopsis:
 *   net.iptables.insert(string table, string chain, string arg1  ...)
 * Description:
 *   init:   iptables -t table -I chain arg1 ...
 *   deinit: iptables -t table -D chain arg1 ...
 * 
 * Synopsis:
 *   net.iptables.policy(string table, string chain, string target, string revert_target)
 * Description:
 *   init:   iptables -t table -P chain target
 *   deinit: iptables -t table -P chain revert_target
 * 
 * Synopsis:
 *   net.iptables.newchain(string table, string chain)
 *   net.iptables.newchain(string chain) // DEPRECATED, defaults to table="filter"
 * Description:
 *   init:   iptables -t table -N chain
 *   deinit: iptables -t table -X chain
 * 
 * Synopsis:
 *   net.ebtables.append(string table, string chain, string arg1 ...)
 * Description:
 *   init:   ebtables -t table -A chain arg1 ...
 *   deinit: ebtables -t table -D chain arg1 ...
 * 
 * Synopsis:
 *   net.ebtables.insert(string table, string chain, string arg1 ...)
 * Description:
 *   init:   ebtables -t table -I chain arg1 ...
 *   deinit: ebtables -t table -D chain arg1 ...
 * 
 * Synopsis:
 *   net.ebtables.policy(string table, string chain, string target, string revert_target)
 * Description:
 *   init:   ebtables -t table -P chain target
 *   deinit: ebtables -t table -P chain revert_target
 * 
 * Synopsis:
 *   net.ebtables.newchain(string table, string chain)
 * Description:
 *   init:   ebtables -t table -N chain
 *   deinit: ebtables -t table -X chain
 * 
 * Synopsis:
 *   net.iptables.lock()
 * Description:
 *   Use at the beginning of a block of custom iptables/ebtables commands to make sure
 *   they do not interfere with other iptables/ebtables commands.
 *   WARNING: improper usage of the lock can lead to deadlock. In particular:
 *   - Do not call any of the iptables/ebtables wrappers above from a lock section;
 *     those will attempt to aquire the lock themselves.
 *   - Do not enter another lock section from a lock section.
 *   - Do not perform any potentially long wait from a lock section.
 * 
 * Synopsis:
 *   net.iptables.lock::unlock()
 * Description:
 *   Use at the end of a block of custom iptables/ebtables commands to make sure
 *   they do not interfere with other iptables/ebtables commands.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <misc/debug.h>
#include <misc/find_program.h>
#include <misc/balloc.h>
#include <ncd/modules/command_template.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_net_iptables.h>

static void template_free_func (void *vo, int is_error);

struct global {
    BEventLock iptables_lock;
};

struct instance {
    NCDModuleInst *i;
    command_template_instance cti;
};

struct unlock_instance;

#define LOCK_STATE_LOCKING 1
#define LOCK_STATE_LOCKED 2
#define LOCK_STATE_UNLOCKED 3
#define LOCK_STATE_RELOCKING 4

struct lock_instance {
    NCDModuleInst *i;
    BEventLockJob lock_job;
    struct unlock_instance *unlock;
    int state;
};

struct unlock_instance {
    NCDModuleInst *i;
    struct lock_instance *lock;
};

static void unlock_free (struct unlock_instance *o);

static int build_append_or_insert_cmdline (NCDModuleInst *i, NCDValRef args, const char *prog, int remove, char **exec, CmdLine *cl, const char *type)
{
    if (NCDVal_ListRead(args, 1, &args) && !NCDVal_IsList(args)) {
        ModuleLog(i, BLOG_ERROR, "in one-argument form a list is expected");
        goto fail0;
    }
    
    // read arguments
    NCDValRef table_arg;
    NCDValRef chain_arg;
    if (!NCDVal_ListReadHead(args, 2, &table_arg, &chain_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(table_arg) || !NCDVal_IsStringNoNulls(chain_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    MemRef table = NCDVal_StringMemRef(table_arg);
    MemRef chain = NCDVal_StringMemRef(chain_arg);
    
    // find program
    if (!(*exec = badvpn_find_program(prog))) {
        ModuleLog(i, BLOG_ERROR, "failed to find program: %s", prog);
        goto fail0;
    }
    
    // start cmdline
    if (!CmdLine_Init(cl)) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Init failed");
        goto fail1;
    }
    
    // add header
    if (!CmdLine_Append(cl, *exec) || !CmdLine_Append(cl, "-t") || !CmdLine_AppendNoNullMr(cl, table) || !CmdLine_Append(cl, (remove ? "-D" : type)) || !CmdLine_AppendNoNullMr(cl, chain)) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Append failed");
        goto fail2;
    }
    
    // add additional arguments
    size_t count = NCDVal_ListCount(args);
    for (size_t j = 2; j < count; j++) {
        NCDValRef arg = NCDVal_ListGet(args, j);
        
        if (!NCDVal_IsStringNoNulls(arg)) {
            ModuleLog(i, BLOG_ERROR, "wrong type");
            goto fail2;
        }
        
        if (!CmdLine_AppendNoNullMr(cl, NCDVal_StringMemRef(arg))) {
            ModuleLog(i, BLOG_ERROR, "CmdLine_AppendNoNullMr failed");
            goto fail2;
        }
    }
    
    // finish
    if (!CmdLine_Finish(cl)) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Finish failed");
        goto fail2;
    }
    
    return 1;
    
fail2:
    CmdLine_Free(cl);
fail1:
    free(*exec);
fail0:
    return 0;
}

static int build_append_cmdline (NCDModuleInst *i, NCDValRef args, const char *prog, int remove, char **exec, CmdLine *cl)
{
    return build_append_or_insert_cmdline(i, args, prog, remove, exec, cl, "-A");
}

static int build_insert_cmdline (NCDModuleInst *i, NCDValRef args, const char *prog, int remove, char **exec, CmdLine *cl)
{
    return build_append_or_insert_cmdline(i, args, prog, remove, exec, cl, "-I");
}

static int build_policy_cmdline (NCDModuleInst *i, NCDValRef args, const char *prog, int remove, char **exec, CmdLine *cl)
{
    // read arguments
    NCDValRef table_arg;
    NCDValRef chain_arg;
    NCDValRef target_arg;
    NCDValRef revert_target_arg;
    if (!NCDVal_ListRead(args, 4, &table_arg, &chain_arg, &target_arg, &revert_target_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(table_arg) || !NCDVal_IsStringNoNulls(chain_arg) ||
        !NCDVal_IsStringNoNulls(target_arg) || !NCDVal_IsStringNoNulls(revert_target_arg)
    ) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    MemRef table = NCDVal_StringMemRef(table_arg);
    MemRef chain = NCDVal_StringMemRef(chain_arg);
    MemRef target = NCDVal_StringMemRef(target_arg);
    MemRef revert_target = NCDVal_StringMemRef(revert_target_arg);
    
    // find program
    if (!(*exec = badvpn_find_program(prog))) {
        ModuleLog(i, BLOG_ERROR, "failed to find program: %s", prog);
        goto fail0;
    }
    
    // start cmdline
    if (!CmdLine_Init(cl)) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Init failed");
        goto fail1;
    }
    
    // add arguments
    if (!CmdLine_Append(cl, *exec) || !CmdLine_Append(cl, "-t") || !CmdLine_AppendNoNullMr(cl, table) ||
        !CmdLine_Append(cl, "-P") || !CmdLine_AppendNoNullMr(cl, chain) ||
        !CmdLine_AppendNoNullMr(cl, (remove ? revert_target : target))
    ) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Append failed");
        goto fail2;
    }
    
    // finish
    if (!CmdLine_Finish(cl)) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Finish failed");
        goto fail2;
    }
    
    return 1;
    
fail2:
    CmdLine_Free(cl);
fail1:
    free(*exec);
fail0:
    return 0;
}

static int build_newchain_cmdline (NCDModuleInst *i, NCDValRef args, const char *prog, int remove, char **exec, CmdLine *cl)
{
    // read arguments
    NCDValRef table_arg = NCDVal_NewInvalid();
    NCDValRef chain_arg;
    if (!NCDVal_ListRead(args, 1, &chain_arg) && !NCDVal_ListRead(args, 2, &table_arg, &chain_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if ((!NCDVal_IsInvalid(table_arg) && !NCDVal_IsStringNoNulls(table_arg)) || !NCDVal_IsStringNoNulls(chain_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    MemRef table = NCDVal_IsInvalid(table_arg) ? MemRef_MakeCstr("filter") : NCDVal_StringMemRef(table_arg);
    MemRef chain = NCDVal_StringMemRef(chain_arg);
    
    // find program
    if (!(*exec = badvpn_find_program(prog))) {
        ModuleLog(i, BLOG_ERROR, "failed to find program: %s", prog);
        goto fail0;
    }
    
    // start cmdline
    if (!CmdLine_Init(cl)) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Init failed");
        goto fail1;
    }
    
    // add arguments
    if (!CmdLine_AppendMulti(cl, 2, *exec, "-t") ||
        !CmdLine_AppendNoNullMr(cl, table) ||
        !CmdLine_Append(cl, (remove ? "-X" : "-N")) ||
        !CmdLine_AppendNoNullMr(cl, chain)
    ) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Append failed");
        goto fail2;
    }
    
    // finish
    if (!CmdLine_Finish(cl)) {
        ModuleLog(i, BLOG_ERROR, "CmdLine_Finish failed");
        goto fail2;
    }
    
    return 1;
    
fail2:
    CmdLine_Free(cl);
fail1:
    free(*exec);
fail0:
    return 0;
}

static int build_iptables_append_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_append_cmdline(i, args, "iptables", remove, exec, cl);
}

static int build_iptables_insert_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_insert_cmdline(i, args, "iptables", remove, exec, cl);
}

static int build_iptables_policy_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_policy_cmdline(i, args, "iptables", remove, exec, cl);
}

static int build_iptables_newchain_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_newchain_cmdline(i, args, "iptables", remove, exec, cl);
}

static int build_ip6tables_append_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_append_cmdline(i, args, "ip6tables", remove, exec, cl);
}

static int build_ip6tables_insert_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_insert_cmdline(i, args, "ip6tables", remove, exec, cl);
}

static int build_ip6tables_policy_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_policy_cmdline(i, args, "ip6tables", remove, exec, cl);
}

static int build_ip6tables_newchain_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_newchain_cmdline(i, args, "ip6tables", remove, exec, cl);
}

static int build_ebtables_append_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_append_cmdline(i, args, "ebtables", remove, exec, cl);
}

static int build_ebtables_insert_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_insert_cmdline(i, args, "ebtables", remove, exec, cl);
}

static int build_ebtables_policy_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_policy_cmdline(i, args, "ebtables", remove, exec, cl);
}

static int build_ebtables_newchain_cmdline (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl)
{
    return build_newchain_cmdline(i, args, "ebtables", remove, exec, cl);
}

static void lock_job_handler (struct lock_instance *o)
{
    ASSERT(o->state == LOCK_STATE_LOCKING || o->state == LOCK_STATE_RELOCKING)
    
    if (o->state == LOCK_STATE_LOCKING) {
        ASSERT(!o->unlock)
        
        // up
        NCDModuleInst_Backend_Up(o->i);
        
        // set state locked
        o->state = LOCK_STATE_LOCKED;
    }
    else if (o->state == LOCK_STATE_RELOCKING) {
        ASSERT(o->unlock)
        ASSERT(o->unlock->lock == o)
        
        // die unlock
        unlock_free(o->unlock);
        o->unlock = NULL;
        
        // set state locked
        o->state = LOCK_STATE_LOCKED;
    }
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
    
    // init iptables lock
    BEventLock_Init(&g->iptables_lock, BReactor_PendingGroup(params->reactor));
    
    return 1;
}

static void func_globalfree (struct NCDInterpModuleGroup *group)
{
    struct global *g = group->group_state;
    
    // free iptables lock
    BEventLock_Free(&g->iptables_lock);
    
    // free global state structure
    BFree(g);
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, command_template_build_cmdline build_cmdline)
{
    struct global *g = ModuleGlobal(i);
    struct instance *o = vo;
    o->i = i;
    
    command_template_new(&o->cti, i, params, build_cmdline, template_free_func, o, BLOG_CURRENT_CHANNEL, &g->iptables_lock);
}

void template_free_func (void *vo, int is_error)
{
    struct instance *o = vo;
    
    if (is_error) {
        NCDModuleInst_Backend_DeadError(o->i);
    } else {
        NCDModuleInst_Backend_Dead(o->i);
    }
}

static void append_iptables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_iptables_append_cmdline);
}

static void insert_iptables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_iptables_insert_cmdline);
}

static void policy_iptables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_iptables_policy_cmdline);
}

static void newchain_iptables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_iptables_newchain_cmdline);
}

static void append_ip6tables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_ip6tables_append_cmdline);
}

static void insert_ip6tables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_ip6tables_insert_cmdline);
}

static void policy_ip6tables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_ip6tables_policy_cmdline);
}

static void newchain_ip6tables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_ip6tables_newchain_cmdline);
}

static void append_ebtables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_ebtables_append_cmdline);
}

static void insert_ebtables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_ebtables_insert_cmdline);
}

static void policy_ebtables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_ebtables_policy_cmdline);
}

static void newchain_ebtables_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    func_new(vo, i, params, build_ebtables_newchain_cmdline);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    
    command_template_die(&o->cti);
}

static void lock_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct global *g = ModuleGlobal(i);
    struct lock_instance *o = vo;
    o->i = i;
    
    // init lock job
    BEventLockJob_Init(&o->lock_job, &g->iptables_lock, (BEventLock_handler)lock_job_handler, o);
    BEventLockJob_Wait(&o->lock_job);
    
    // set no unlock
    o->unlock = NULL;
    
    // set state locking
    o->state = LOCK_STATE_LOCKING;
}

static void lock_func_die (void *vo)
{
    struct lock_instance *o = vo;
    
    if (o->state == LOCK_STATE_UNLOCKED) {
        ASSERT(o->unlock)
        ASSERT(o->unlock->lock == o)
        o->unlock->lock = NULL;
    }
    else if (o->state == LOCK_STATE_RELOCKING) {
        ASSERT(o->unlock)
        ASSERT(o->unlock->lock == o)
        unlock_free(o->unlock);
    }
    else {
        ASSERT(!o->unlock)
    }
    
    // free lock job
    BEventLockJob_Free(&o->lock_job);
    
    // dead
    NCDModuleInst_Backend_Dead(o->i);
}

static void unlock_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct unlock_instance *o = vo;
    o->i = i;
    
    // get lock lock
    struct lock_instance *lock = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // make sure lock doesn't already have an unlock
    if (lock->unlock) {
        BLog(BLOG_ERROR, "lock already has an unlock");
        goto fail0;
    }
    
    // make sure lock is locked
    if (lock->state != LOCK_STATE_LOCKED) {
        BLog(BLOG_ERROR, "lock is not locked");
        goto fail0;
    }
    
    // set lock
    o->lock = lock;
    
    // set unlock in lock
    lock->unlock = o;
    
    // up
    NCDModuleInst_Backend_Up(o->i);
    
    // release lock
    BEventLockJob_Release(&lock->lock_job);
    
    // set lock state unlocked
    lock->state = LOCK_STATE_UNLOCKED;
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void unlock_func_die (void *vo)
{
    struct unlock_instance *o = vo;
    
    // if lock is gone, die right away
    if (!o->lock) {
        unlock_free(o);
        return;
    }
    
    ASSERT(o->lock->unlock == o)
    ASSERT(o->lock->state == LOCK_STATE_UNLOCKED)
    
    // wait lock
    BEventLockJob_Wait(&o->lock->lock_job);
    
    // set lock state relocking
    o->lock->state = LOCK_STATE_RELOCKING;
}

static void unlock_free (struct unlock_instance *o)
{
    NCDModuleInst_Backend_Dead(o->i);
}

static struct NCDModule modules[] = {
    {
        .type = "net.iptables.append",
        .func_new2 = append_iptables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.iptables.insert",
        .func_new2 = insert_iptables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.iptables.policy",
        .func_new2 = policy_iptables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.iptables.newchain",
        .func_new2 = newchain_iptables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.ip6tables.append",
        .func_new2 = append_ip6tables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.ip6tables.insert",
        .func_new2 = insert_ip6tables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.ip6tables.policy",
        .func_new2 = policy_ip6tables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.ip6tables.newchain",
        .func_new2 = newchain_ip6tables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.ebtables.append",
        .func_new2 = append_ebtables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.ebtables.insert",
        .func_new2 = insert_ebtables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.ebtables.policy",
        .func_new2 = policy_ebtables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.ebtables.newchain",
        .func_new2 = newchain_ebtables_func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "net.iptables.lock",
        .func_new2 = lock_func_new,
        .func_die = lock_func_die,
        .alloc_size = sizeof(struct lock_instance)
    }, {
        .type = "net.iptables.lock::unlock",
        .func_new2 = unlock_func_new,
        .func_die = unlock_func_die,
        .alloc_size = sizeof(struct unlock_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_net_iptables = {
    .modules = modules,
    .func_globalinit = func_globalinit,
    .func_globalfree = func_globalfree
};
