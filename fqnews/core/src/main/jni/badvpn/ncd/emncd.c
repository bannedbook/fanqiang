/**
 * @file emncd.c
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

#include <misc/version.h>
#include <misc/debug.h>
#include <base/BLog.h>
#include <system/BTime.h>
#include <system/BReactor.h>
#include <ncd/NCDInterpreter.h>
#include <ncd/NCDConfigParser.h>

#include <generated/blog_channel_ncd.h>

static BReactor reactor;
static int running;
static NCDInterpreter interpreter;

static void interpreter_handler_finished (void *user, int exit_code)
{
    ASSERT(running)
    
    fprintf(stderr, "--- interpreter terminated ---\n");
    
    NCDInterpreter_Free(&interpreter);
    
    running = 0;
}

__attribute__((used))
int main ()
{
    BLog_InitStderr();
    
    fprintf(stderr, "--- initializing emncd version "GLOBAL_VERSION" ---\n");
    
    BTime_Init();
    
    BReactor_EmscriptenInit(&reactor);
    
    running = 0;
    
    return 0;
}

__attribute__((used))
void emncd_start (const char *program_text, int loglevel)
{
    ASSERT(program_text);
    ASSERT(loglevel >= 0);
    ASSERT(loglevel <= BLOG_DEBUG);
    
    if (running) {
        fprintf(stderr, "--- cannot start, interpreter is already running! ---\n");
        return;
    }
    
    for (int i = 0; i < BLOG_NUM_CHANNELS; i++) {
        BLog_SetChannelLoglevel(i, loglevel);
    }
    
    // parse program
    NCDProgram program;
    if (!NCDConfigParser_Parse((char *)program_text ,strlen(program_text), &program)) {
        fprintf(stderr, "--- error: failed to parse the program ---\n");
        return;
    }
    
    // include commands are not implemented currently
    if (NCDProgram_ContainsElemType(&program, NCDPROGRAMELEM_INCLUDE) || NCDProgram_ContainsElemType(&program, NCDPROGRAMELEM_INCLUDE_GUARD)) {
        fprintf(stderr, "--- error: include mechanism is not supported in emncd ---\n");
        NCDProgram_Free(&program);
        return;
    }
    
    fprintf(stderr, "--- starting interpreter ---\n");
    
    struct NCDInterpreter_params params;
    params.handler_finished = interpreter_handler_finished;
    params.user = NULL;
    params.retry_time = 5000;
    params.extra_args = NULL;
    params.num_extra_args = 0;
    params.reactor = &reactor;
    
    if (!NCDInterpreter_Init(&interpreter, program, params)) {
        fprintf(stderr, "--- failed to initialize the interpreter ---\n");
        return;
    }
    
    running = 1;
    
    BReactor_EmscriptenSync(&reactor);
}

__attribute__((used))
void emncd_stop (void)
{
    if (!running) {
        fprintf(stderr, "--- cannot request termination, interpreter is not running! ---\n");
        return;
    }
    
    fprintf(stderr, "--- requesting interpreter termination ---\n");
    
    NCDInterpreter_RequestShutdown(&interpreter, 0);
    
    BReactor_EmscriptenSync(&reactor);
}
