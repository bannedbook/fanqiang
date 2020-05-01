/**
 * @file build_cmdline.c
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

#include <misc/debug.h>
#include <ncd/extra/value_utils.h>

#include "build_cmdline.h"

int ncd_build_cmdline (NCDModuleInst *i, int log_channel, NCDValRef cmd_arg, char **out_exec, CmdLine *out_cl)
{
    ASSERT(!NCDVal_IsInvalid(cmd_arg))
    ASSERT(out_exec)
    ASSERT(out_cl)
    
    if (!NCDVal_IsList(cmd_arg)) {
        NCDModuleInst_Backend_Log(i, log_channel, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    size_t count = NCDVal_ListCount(cmd_arg);
    
    // read exec
    if (count == 0) {
        NCDModuleInst_Backend_Log(i, log_channel, BLOG_ERROR, "missing executable name");
        goto fail0;
    }
    NCDValRef exec_arg = NCDVal_ListGet(cmd_arg, 0);
    if (!NCDVal_IsStringNoNulls(exec_arg)) {
        NCDModuleInst_Backend_Log(i, log_channel, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    char *exec = ncd_strdup(exec_arg);
    if (!exec) {
        NCDModuleInst_Backend_Log(i, log_channel, BLOG_ERROR, "ncd_strdup failed");
        goto fail0;
    }
    
    // start cmdline
    CmdLine cl;
    if (!CmdLine_Init(&cl)) {
        NCDModuleInst_Backend_Log(i, log_channel, BLOG_ERROR, "CmdLine_Init failed");
        goto fail1;
    }
    
    // add header
    if (!CmdLine_Append(&cl, exec)) {
        NCDModuleInst_Backend_Log(i, log_channel, BLOG_ERROR, "CmdLine_Append failed");
        goto fail2;
    }
    
    // add additional arguments
    for (size_t j = 1; j < count; j++) {
        NCDValRef arg = NCDVal_ListGet(cmd_arg, j);
        
        if (!NCDVal_IsStringNoNulls(arg)) {
            NCDModuleInst_Backend_Log(i, log_channel, BLOG_ERROR, "wrong type");
            goto fail2;
        }
        
        if (!CmdLine_AppendNoNullMr(&cl, NCDVal_StringMemRef(arg))) {
            NCDModuleInst_Backend_Log(i, log_channel, BLOG_ERROR, "CmdLine_AppendNoNullMr failed");
            goto fail2;
        }
    }
    
    // finish
    if (!CmdLine_Finish(&cl)) {
        NCDModuleInst_Backend_Log(i, log_channel, BLOG_ERROR, "CmdLine_Finish failed");
        goto fail2;
    }
    
    *out_exec = exec;
    *out_cl = cl;
    return 1;
    
fail2:
    CmdLine_Free(&cl);
fail1:
    free(exec);
fail0:
    return 0;
}
