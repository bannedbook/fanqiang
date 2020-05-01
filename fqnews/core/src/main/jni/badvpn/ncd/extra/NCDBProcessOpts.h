/**
 * @file NCDBProcessOpts.h
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

#ifndef NCD_BPROCESS_OPTS_H
#define NCD_BPROCESS_OPTS_H

#include <misc/debug.h>
#include <ncd/NCDModule.h>
#include <system/BProcess.h>

typedef struct {
    char *username;
    int do_setsid;
    int fds[3];
    int fds_map[2];
    int nfds;
} NCDBProcessOpts;

typedef int (*NCDBProcessOpts_func_unknown) (void *user, NCDValRef key, NCDValRef val);

int NCDBProcessOpts_Init (NCDBProcessOpts *o, NCDValRef opts_arg, NCDBProcessOpts_func_unknown func_unknown, void *func_unknown_user, NCDModuleInst *i, int blog_channel) WARN_UNUSED;
int NCDBProcessOpts_Init2 (NCDBProcessOpts *o, NCDValRef opts_arg, NCDBProcessOpts_func_unknown func_unknown, void *func_unknown_user, NCDModuleInst *i, int blog_channel,
                           int *out_keep_stdout, int *out_keep_stderr) WARN_UNUSED;
void NCDBProcessOpts_InitOld (NCDBProcessOpts *o, int keep_stdout, int keep_stderr, int do_setsid);
void NCDBProcessOpts_Free (NCDBProcessOpts *o);
struct BProcess_params NCDBProcessOpts_GetParams (NCDBProcessOpts *o);

#endif
