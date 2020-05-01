/**
 * @file log.c
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
 * (INCLUDING, BUT NOT LIMITED TO OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * @section DESCRIPTION
 * 
 * Message logging using the BLog system provided by the BadVPN framework.
 * Each message has an associated loglevel, which must be one of: "error, "warning",
 * "notice", "info", "debug", or a numeric identifier (1=error to 5=debug).
 * 
 * Synopsis:
 *   log(string level [, string ...])
 * 
 * Description:
 *   On init, logs the concatenation of the given strings.
 * 
 * Synopsis:
 *   log_r(string level [, string ...])
 * 
 * Description:
 *   On deinit, logs the concatenation of the given strings.
 * 
 * Synopsis:
 *   log_fr(string level, list(string) strings_init, list(string) strings_deinit)
 * 
 * Description:
 *   On init, logs the concatenation of the strings in 'strings_init',
 *   and on deinit, logs the concatenation of the strings in 'strings_deinit'.
 */

#include <stdlib.h>
#include <stdio.h>

#include <misc/debug.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_log.h>

struct rlog_instance {
    NCDModuleInst *i;
    int level;
    NCDValRef list;
    size_t start;
};

enum {STRING_ERROR, STRING_WARNING, STRING_NOTICE, STRING_INFO, STRING_DEBUG};

static const char *strings[] = {
    "error", "warning", "notice", "info", "debug", NULL
};

static int check_strings (NCDValRef list, size_t start)
{
    ASSERT(NCDVal_IsList(list))
    
    size_t count = NCDVal_ListCount(list);
    
    for (size_t j = start; j < count; j++) {
        NCDValRef string = NCDVal_ListGet(list, j);
        if (!NCDVal_IsString(string)) {
            return 0;
        }
    }
    
    return 1;
}

static void do_log (int level, NCDValRef list, size_t start)
{
    ASSERT(level >= BLOG_ERROR)
    ASSERT(level <= BLOG_DEBUG)
    ASSERT(check_strings(list, start))
    
    if (!BLog_WouldLog(BLOG_CHANNEL_ncd_log_msg, level)) {
        return;
    }
    
    size_t count = NCDVal_ListCount(list);
    
    BLog_Begin();
    
    for (size_t j = start; j < count; j++) {
        NCDValRef string = NCDVal_ListGet(list, j);
        ASSERT(NCDVal_IsString(string))
        BLog_AppendBytes(NCDVal_StringMemRef(string));
    }
    
    BLog_Finish(BLOG_CHANNEL_ncd_log_msg, level);
}

static int parse_level (NCDModuleInst *i, NCDValRef level_arg, int *out_level)
{
    int found = 0;
    if (NCDVal_IsString(level_arg)) {
        found = 1;
        
        if (NCDVal_StringEqualsId(level_arg, ModuleString(i, STRING_ERROR))) {
            *out_level = BLOG_ERROR;
        }
        else if (NCDVal_StringEqualsId(level_arg, ModuleString(i, STRING_WARNING))) {
            *out_level = BLOG_WARNING;
        }
        else if (NCDVal_StringEqualsId(level_arg, ModuleString(i, STRING_NOTICE))) {
            *out_level = BLOG_NOTICE;
        }
        else if (NCDVal_StringEqualsId(level_arg, ModuleString(i, STRING_INFO))) {
            *out_level = BLOG_INFO;
        }
        else if (NCDVal_StringEqualsId(level_arg, ModuleString(i, STRING_DEBUG))) {
            *out_level = BLOG_DEBUG;
        }
        else {
            found = 0;
        }
    }
    
    if (!found) {
        uintmax_t level_numeric;
        if (!ncd_read_uintmax(level_arg, &level_numeric) || !(level_numeric >= BLOG_ERROR && level_numeric <= BLOG_DEBUG)) {
            return 0;
        }
        *out_level = level_numeric;
    }
    
    return 1;
}

static void rlog_func_new_common (void *vo, NCDModuleInst *i, int level, NCDValRef list, size_t start)
{
    ASSERT(level >= BLOG_ERROR)
    ASSERT(level <= BLOG_DEBUG)
    ASSERT(check_strings(list, start))
    
    struct rlog_instance *o = vo;
    o->i = i;
    o->level = level;
    o->list = list;
    o->start = start;
    
    NCDModuleInst_Backend_Up(i);
}

static void rlog_func_die (void *vo)
{
    struct rlog_instance *o = vo;
    
    do_log(o->level, o->list, o->start);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void log_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    if (NCDVal_ListCount(params->args) < 1) {
        ModuleLog(i, BLOG_ERROR, "missing level argument");
        goto fail0;
    }
    
    int level;
    if (!parse_level(i, NCDVal_ListGet(params->args, 0), &level)) {
        ModuleLog(i, BLOG_ERROR, "wrong level argument");
        goto fail0;
    }
    
    if (!check_strings(params->args, 1)) {
        ModuleLog(i, BLOG_ERROR, "wrong string arguments");
        goto fail0;
    }
    
    do_log(level, params->args, 1);
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void log_r_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    if (NCDVal_ListCount(params->args) < 1) {
        ModuleLog(i, BLOG_ERROR, "missing level argument");
        goto fail0;
    }
    
    int level;
    if (!parse_level(i, NCDVal_ListGet(params->args, 0), &level)) {
        ModuleLog(i, BLOG_ERROR, "wrong level argument");
        goto fail0;
    }
    
    if (!check_strings(params->args, 1)) {
        ModuleLog(i, BLOG_ERROR, "wrong string arguments");
        goto fail0;
    }
    
    rlog_func_new_common(vo, i, level, params->args, 1);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void log_fr_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef level_arg;
    NCDValRef strings_init_arg;
    NCDValRef strings_deinit_arg;
    if (!NCDVal_ListRead(params->args, 3, &level_arg, &strings_init_arg, &strings_deinit_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    int level;
    if (!parse_level(i, level_arg, &level)) {
        ModuleLog(i, BLOG_ERROR, "wrong level argument");
        goto fail0;
    }
    
    if (!NCDVal_IsList(strings_init_arg) || !check_strings(strings_init_arg, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong string_init argument");
        goto fail0;
    }
    
    if (!NCDVal_IsList(strings_deinit_arg) || !check_strings(strings_deinit_arg, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong strings_deinit argument");
        goto fail0;
    }
    
    do_log(level, strings_init_arg, 0);
    
    rlog_func_new_common(vo, i, level, strings_deinit_arg, 0);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "log",
        .func_new2 = log_func_new
    }, {
        .type = "log_r",
        .func_new2 = log_r_func_new,
        .func_die = rlog_func_die,
        .alloc_size = sizeof(struct rlog_instance)
    }, {
        .type = "log_fr",
        .func_new2 = log_fr_func_new,
        .func_die = rlog_func_die,
        .alloc_size = sizeof(struct rlog_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_log = {
    .modules = modules,
    .strings = strings
};
