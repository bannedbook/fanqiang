/**
 * @file NCDBProcessOpts.c
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

#include <stddef.h>

#include <misc/balloc.h>

#include <ncd/extra/value_utils.h>

#include "NCDBProcessOpts.h"

int NCDBProcessOpts_Init (NCDBProcessOpts *o, NCDValRef opts_arg, NCDBProcessOpts_func_unknown func_unknown, void *func_unknown_user, NCDModuleInst *i, int blog_channel)
{
    return NCDBProcessOpts_Init2(o, opts_arg, func_unknown, func_unknown_user, i, blog_channel, NULL, NULL);
}

int NCDBProcessOpts_Init2 (NCDBProcessOpts *o, NCDValRef opts_arg, NCDBProcessOpts_func_unknown func_unknown, void *func_unknown_user, NCDModuleInst *i, int blog_channel,
                           int *out_keep_stdout, int *out_keep_stderr)
{
    if (!NCDVal_IsInvalid(opts_arg) && !NCDVal_IsMap(opts_arg)) {
        NCDModuleInst_Backend_Log(i, blog_channel, BLOG_ERROR, "options must be a map");
        goto fail0;
    }
    
    o->username = NULL;
    o->do_setsid = 0;
    
    int keep_stdout = 0;
    int keep_stderr = 0;
    
    if (!NCDVal_IsInvalid(opts_arg)) {
        for (NCDValMapElem me = NCDVal_MapFirst(opts_arg); !NCDVal_MapElemInvalid(me); me = NCDVal_MapNext(opts_arg, me)) {
            NCDValRef key = NCDVal_MapElemKey(opts_arg, me);
            NCDValRef val = NCDVal_MapElemVal(opts_arg, me);
            
            if (NCDVal_IsString(key) && NCDVal_StringEquals(key, "keep_stdout")) {
                if (!ncd_read_boolean(val, &keep_stdout)) {
                    NCDModuleInst_Backend_Log(i, blog_channel, BLOG_ERROR, "bad keep_stdout");
                    goto fail1;
                }
            }
            else if (NCDVal_IsString(key) && NCDVal_StringEquals(key, "keep_stderr")) {
                if (!ncd_read_boolean(val, &keep_stderr)) {
                    NCDModuleInst_Backend_Log(i, blog_channel, BLOG_ERROR, "bad keep_stderr");
                    goto fail1;
                }
            }
            else if (NCDVal_IsString(key) && NCDVal_StringEquals(key, "do_setsid")) {
                if (!ncd_read_boolean(val, &o->do_setsid)) {
                    NCDModuleInst_Backend_Log(i, blog_channel, BLOG_ERROR, "bad do_setsid");
                    goto fail1;
                }
            }
            else if (NCDVal_IsString(key) && NCDVal_StringEquals(key, "username")) {
                if (!NCDVal_IsStringNoNulls(val)) {
                    NCDModuleInst_Backend_Log(i, blog_channel, BLOG_ERROR, "username must be a string without nulls");
                    goto fail1;
                }
                o->username = MemRef_StrDup(NCDVal_StringMemRef(val));
                if (!o->username) {
                    NCDModuleInst_Backend_Log(i, blog_channel, BLOG_ERROR, "MemRef_StrDup failed");
                    goto fail1;
                }
            }
            else {
                if (!func_unknown || !func_unknown(func_unknown_user, key, val)) {
                    NCDModuleInst_Backend_Log(i, blog_channel, BLOG_ERROR, "unknown option");
                    goto fail1;
                }
            }
        }
    }
    
    o->nfds = 0;
    if (keep_stdout) {
        o->fds[o->nfds] = 1;
        o->fds_map[o->nfds++] = 1;
    }
    if (keep_stderr) {
        o->fds[o->nfds] = 2;
        o->fds_map[o->nfds++] = 2;
    }
    o->fds[o->nfds] = -1;
    
    if (out_keep_stdout) {
        *out_keep_stdout = keep_stdout;
    }
    if (out_keep_stderr) {
        *out_keep_stderr = keep_stderr;
    }
    
    return 1;
    
fail1:
    if (o->username) {
        BFree(o->username);
    }
fail0:
    return 0;
}

void NCDBProcessOpts_InitOld (NCDBProcessOpts *o, int keep_stdout, int keep_stderr, int do_setsid)
{
    o->username = NULL;
    o->do_setsid = do_setsid;
    
    o->nfds = 0;
    if (keep_stdout) {
        o->fds[o->nfds] = 1;
        o->fds_map[o->nfds++] = 1;
    }
    if (keep_stderr) {
        o->fds[o->nfds] = 2;
        o->fds_map[o->nfds++] = 2;
    }
    o->fds[o->nfds] = -1;
}

void NCDBProcessOpts_Free (NCDBProcessOpts *o)
{
    if (o->username) {
        BFree(o->username);
    }
}

struct BProcess_params NCDBProcessOpts_GetParams (NCDBProcessOpts *o)
{
    struct BProcess_params params;
    
    params.username = o->username;
    params.fds = o->fds;
    params.fds_map = o->fds_map;
    params.do_setsid = o->do_setsid;
    
    return params;
}
