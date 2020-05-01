/**
 * @file NCDInterpreter.h
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

#ifndef BADVPN_NCD_INTERPRETER_H
#define BADVPN_NCD_INTERPRETER_H

#include <stddef.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <system/BTime.h>
#include <system/BReactor.h>
#include <ncd/NCDStringIndex.h>
#include <ncd/NCDModuleIndex.h>
#include <ncd/NCDAst.h>
#include <ncd/NCDEvaluator.h>
#include <ncd/NCDInterpProg.h>
#include <ncd/NCDModule.h>
#include <structure/LinkedList1.h>

#ifndef BADVPN_NO_PROCESS
#include <system/BProcess.h>
#endif
#ifndef BADVPN_NO_UDEV
#include <udevmonitor/NCDUdevManager.h>
#endif
#ifndef BADVPN_NO_RANDOM
#include <random/BRandom2.h>
#endif

/**
 * Handler called when the interpreter has terminated, and {@link NCDInterpreter_Free}
 * can be called.
 * 
 * @param user the user member of struct {@link NCDInterpreter_params}
 * @param exit_code the exit code specified in the last interpreter termination request
 */
typedef void (*NCDInterpreter_handler_finished) (void *user, int exit_code);

struct NCDInterpreter_params {
    // callbacks
    NCDInterpreter_handler_finished handler_finished;
    void *user;
    
    // options
    btime_t retry_time;
    char **extra_args;
    int num_extra_args;
    
    // possibly shared resources
    BReactor *reactor;
#ifndef BADVPN_NO_PROCESS
    BProcessManager *manager;
#endif
#ifndef BADVPN_NO_UDEV
    NCDUdevManager *umanager;
#endif
#ifndef BADVPN_NO_RANDOM
    BRandom2 *random2;
#endif
};

typedef struct {
    // parameters
    struct NCDInterpreter_params params;
    
    // are we terminating
    int terminating;
    int main_exit_code;

    // string index
    NCDStringIndex string_index;

    // module index
    NCDModuleIndex mindex;

    // program AST
    NCDProgram program;

    // expression evaluator
    NCDEvaluator evaluator;

    // structure for efficient interpretation
    NCDInterpProg iprogram;

    // common module parameters
    struct NCDModuleInst_params module_params;
    struct NCDModuleInst_iparams module_iparams;
    struct NCDCall_interp_shared module_call_shared;
    
    // processes
    LinkedList1 processes;
    
    DebugObject d_obj;
} NCDInterpreter;

/**
 * Initializes and starts the interpreter.
 * 
 * @param o the interpreter
 * @param program the program to execute in AST format. The program must
 *                not contain any 'include' or 'include_guard' directives.
 *                The interpreter takes ownership of the program, regardless
 *                of the success of this function.
 * @param params other parameters
 * @return 1 on success, 0 on failure
 */
int NCDInterpreter_Init (NCDInterpreter *o, NCDProgram program, struct NCDInterpreter_params params) WARN_UNUSED;

/**
 * Frees the interpreter.
 * This may only be called after the interpreter has terminated, i.e.
 * the {@link NCDInterpreter_handler_finished} handler has been called.
 * Additionally, it can be called right after {@link NCDInterpreter_Init}
 * before any of the interpreter's {@link BPendingGroup} jobs have executed.
 */
void NCDInterpreter_Free (NCDInterpreter *o);

/**
 * Requests termination of the interpreter.
 * NOTE: the program can request its own termination, possibly overriding the exit
 * code specified here. Expect the program to terminate even if this function was
 * not called.
 * 
 * @param o the interpreter
 * @param exit_code the exit code to be passed to {@link NCDInterpreter_handler_finished}.
 *                  This overrides any exit code set previously.
 */
void NCDInterpreter_RequestShutdown (NCDInterpreter *o, int exit_code);

#endif
