/**
 * @file command_template.h
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
 * Template for a module which executes a command to start and stop.
 * The command is executed asynchronously.
 */

#ifndef BADVPN_NCD_MODULES_COMMAND_TEMPLATE_H
#define BADVPN_NCD_MODULES_COMMAND_TEMPLATE_H

#include <misc/cmdline.h>
#include <ncd/NCDModule.h>
#include <ncd/extra/BEventLock.h>

typedef int (*command_template_build_cmdline) (NCDModuleInst *i, NCDValRef args, int remove, char **exec, CmdLine *cl);
typedef void (*command_template_free_func) (void *user, int is_error);

typedef struct {
    NCDModuleInst *i;
    command_template_free_func free_func;
    void *user;
    int blog_channel;
    char *do_exec;
    CmdLine do_cmdline;
    char *undo_exec;
    CmdLine undo_cmdline;
    BEventLockJob elock_job;
    int state;
    BProcess process;
} command_template_instance;

void command_template_new (command_template_instance *o, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, command_template_build_cmdline build_cmdline, command_template_free_func free_func, void *user, int blog_channel, BEventLock *elock);
void command_template_die (command_template_instance *o);

#endif
