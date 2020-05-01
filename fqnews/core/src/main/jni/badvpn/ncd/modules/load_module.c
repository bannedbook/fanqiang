/**
 * @file load_module.c
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
 * Synopsis:
 *   load_module(string name)
 */

#include <limits.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>

#include <misc/balloc.h>
#include <misc/concat_strings.h>
#include <misc/strdup.h>
#include <misc/offset.h>
#include <misc/debug.h>
#include <structure/LinkedList0.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_load_module.h>

struct global {
    LinkedList0 modules_list;
};

struct module {
    char *name;
    void *lib_handle;
    int ncdmodule_loaded;
    LinkedList0Node modules_list_node;
};

static struct module * find_module (const char *name, struct global *g)
{
    for (LinkedList0Node *ln = LinkedList0_GetFirst(&g->modules_list); ln; ln = LinkedList0Node_Next(ln)) {
        struct module *mod = UPPER_OBJECT(ln, struct module, modules_list_node);
        if (!strcmp(mod->name, name)) {
            return mod;
        }
    }
    return NULL;
}

static struct module * module_init (const char *name, NCDModuleInst *i)
{
    struct global *g = ModuleGlobal(i);
    ASSERT(!find_module(name, g))
    
    struct module *mod = BAlloc(sizeof(*mod));
    if (!mod) {
        ModuleLog(i, BLOG_ERROR, "BAlloc failed");
        goto fail0;
    }
    
    mod->name = b_strdup(name);
    if (!mod->name) {
        ModuleLog(i, BLOG_ERROR, "b_strdup failed");
        goto fail1;
    }
    
    mod->lib_handle = NULL;
    mod->ncdmodule_loaded = 0;
    LinkedList0_Prepend(&g->modules_list, &mod->modules_list_node);
    
    return mod;
    
fail1:
    BFree(mod);
fail0:
    return NULL;
}

static void module_free (struct module *mod, struct global *g)
{
    LinkedList0_Remove(&g->modules_list, &mod->modules_list_node);
    if (mod->lib_handle) {
        if (dlclose(mod->lib_handle) != 0) {
            BLog(BLOG_ERROR, "dlclose failed");
        }
    }
    BFree(mod->name);
    BFree(mod);
}

static char * x_read_link (const char *path)
{
    size_t size = 32;
    char *buf = BAlloc(size + 1);
    if (!buf) {
        goto fail0;
    }
    
    ssize_t link_size;
    while (1) {
        link_size = readlink(path, buf, size);
        if (link_size < 0) {
            goto fail1;
        }
        if (link_size >= 0 && link_size < size) {
            break;
        }
        if (size > SIZE_MAX / 2 || 2 * size > SIZE_MAX - 1) {
            goto fail1;
        }
        size *= 2;
        char *new_buf = BRealloc(buf, size + 1);
        if (!new_buf) {
            goto fail1;
        }
        buf = new_buf;
    }
    
    buf[link_size] = '\0';
    return buf;
    
fail1:
    BFree(buf);
fail0:
    return NULL;
}

static char * find_module_library (NCDModuleInst *i, const char *module_name)
{
    char *ret = NULL;
    
    char *self = x_read_link("/proc/self/exe");
    if (!self) {
        ModuleLog(i, BLOG_ERROR, "failed to read /proc/self/exe");
        goto fail0;
    }
    
    char *slash = strrchr(self, '/');
    if (!slash) {
        ModuleLog(i, BLOG_ERROR, "contents of /proc/self/exe do not have a slash");
        goto fail1;
    }
    *slash = '\0';
    
    const char *paths[] = {"../lib/badvpn-ncd", "../mcvpn", NULL};
    
    size_t j;
    for (j = 0; paths[j]; j++) {
        char *module_path = concat_strings(6, self, "/", paths[j], "/libncdmodule_", module_name, ".so");
        if (!module_path) {
            ModuleLog(i, BLOG_ERROR, "concat_strings failed");
            goto fail1;
        }
        
        if (access(module_path, F_OK) == 0) {
            ret = module_path;
            break;
        }
        
        BFree(module_path);
    }
    
    if (!paths[j]) {
        ModuleLog(i, BLOG_ERROR, "failed to find module");
    }
    
fail1:
    BFree(self);
fail0:
    return ret;
}

static int func_globalinit (struct NCDInterpModuleGroup *group, const struct NCDModuleInst_iparams *params)
{
    struct global *g = BAlloc(sizeof(*g));
    if (!g) {
        BLog(BLOG_ERROR, "BAlloc failed");
        return 0;
    }
    
    group->group_state = g;
    LinkedList0_Init(&g->modules_list);
    
    return 1;
}

static void func_globalfree (struct NCDInterpModuleGroup *group)
{
    struct global *g = group->group_state;
    
    LinkedList0Node *ln;
    while ((ln = LinkedList0_GetFirst(&g->modules_list))) {
        struct module *mod = UPPER_OBJECT(ln, struct module, modules_list_node);
        module_free(mod, g);
    }
    
    BFree(g);
}

static void func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    NCDValRef name_arg;
    if (!NCDVal_ListRead(params->args, 1, &name_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(name_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    NCDValNullTermString name_nts;
    if (!NCDVal_StringNullTerminate(name_arg, &name_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    struct module *mod = find_module(name_nts.data, ModuleGlobal(i));
    ASSERT(!mod || mod->lib_handle)
    
    if (!mod) {
        mod = module_init(name_nts.data, i);
        if (!mod) {
            ModuleLog(i, BLOG_ERROR, "module_init failed");
            goto fail1;
        }
        
        // find module library
        char *module_path = find_module_library(i, name_nts.data);
        if (!module_path) {
            module_free(mod, ModuleGlobal(i));
            goto fail1;
        }
        
        // load it as a dynamic library
        mod->lib_handle = dlopen(module_path, RTLD_NOW);
        BFree(module_path);
        if (!mod->lib_handle) {
            ModuleLog(i, BLOG_ERROR, "dlopen failed");
            module_free(mod, ModuleGlobal(i));
            goto fail1;
        }
    }
    
    if (!mod->ncdmodule_loaded) {
        // build name of NCDModuleGroup structure symbol
        char *group_symbol = concat_strings(2, "ncdmodule_", name_nts.data);
        if (!group_symbol) {
            ModuleLog(i, BLOG_ERROR, "concat_strings failed");
            goto fail1;
        }
        
        // resolve NCDModuleGroup structure symbol
        void *group = dlsym(mod->lib_handle, group_symbol);
        BFree(group_symbol);
        if (!group) {
            ModuleLog(i, BLOG_ERROR, "dlsym failed");
            goto fail1;
        }
        
        // load module group
        if (!NCDModuleInst_Backend_InterpLoadGroup(i, (struct NCDModuleGroup *)group)) {
            ModuleLog(i, BLOG_ERROR, "NCDModuleInst_Backend_InterpLoadGroup failed");
            goto fail1;
        }
        
        mod->ncdmodule_loaded = 1;
    }
    
    NCDValNullTermString_Free(&name_nts);
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail1:
    NCDValNullTermString_Free(&name_nts);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "load_module",
        .func_new2 = func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_load_module = {
    .func_globalinit = func_globalinit,
    .func_globalfree = func_globalfree,
    .modules = modules
};
