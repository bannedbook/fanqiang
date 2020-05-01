/**
 * @file ncd.c
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <misc/version.h>
#include <misc/loglevel.h>
#include <misc/open_standard_streams.h>
#include <misc/string_begins_with.h>
#include <base/BLog.h>
#include <system/BReactor.h>
#include <system/BSignal.h>
#include <system/BProcess.h>
#include <udevmonitor/NCDUdevManager.h>
#include <random/BRandom2.h>
#include <ncd/NCDInterpreter.h>
#include <ncd/NCDBuildProgram.h>

#ifdef BADVPN_USE_SYSLOG
#include <base/BLog_syslog.h>
#endif

#include "ncd.h"

#include <generated/blog_channel_ncd.h>

#define LOGGER_STDOUT 1
#define LOGGER_STDERR 2
#define LOGGER_SYSLOG 3

// command-line options
static struct {
    int help;
    int version;
    int logger;
#ifdef BADVPN_USE_SYSLOG
    char *logger_syslog_facility;
    char *logger_syslog_ident;
#endif
    int loglevel;
    int loglevels[BLOG_NUM_CHANNELS];
    char *config_file;
    int syntax_only;
    int retry_time;
    int signal_exit_code;
    int no_udev;
    char **extra_args;
    int num_extra_args;
} options;

// reactor
static BReactor reactor;

// process manager
static BProcessManager manager;

// udev manager
static NCDUdevManager umanager;

// random number generator
static BRandom2 random2;

// interpreter
static NCDInterpreter interpreter;

// forward declarations of functions
static void print_help (const char *name);
static void print_version (void);
static int parse_arguments (int argc, char *argv[]);
static void signal_handler (void *unused);
static void interpreter_handler_finished (void *user, int exit_code);

int main (int argc, char **argv)
{
    if (argc <= 0) {
        return 1;
    }
    
    int main_exit_code = 1;
    
    // open standard streams
    open_standard_streams();
    
    // parse command-line arguments
    if (!parse_arguments(argc, argv)) {
        fprintf(stderr, "Failed to parse arguments\n");
        print_help(argv[0]);
        goto fail0;
    }
    
    // handle --help and --version
    if (options.help) {
        print_version();
        print_help(argv[0]);
        return 0;
    }
    if (options.version) {
        print_version();
        return 0;
    }
    
    // initialize logger
    switch (options.logger) {
        case LOGGER_STDOUT:
            BLog_InitStdout();
            break;
        case LOGGER_STDERR:
            BLog_InitStderr();
            break;
#ifdef BADVPN_USE_SYSLOG
        case LOGGER_SYSLOG:
            if (!BLog_InitSyslog(options.logger_syslog_ident, options.logger_syslog_facility)) {
                fprintf(stderr, "Failed to initialize syslog logger\n");
                goto fail0;
            }
            break;
#endif
        default:
            ASSERT(0);
    }
    
    // configure logger channels
    for (int i = 0; i < BLOG_NUM_CHANNELS; i++) {
        if (options.loglevels[i] >= 0) {
            BLog_SetChannelLoglevel(i, options.loglevels[i]);
        }
        else if (options.loglevel >= 0) {
            BLog_SetChannelLoglevel(i, options.loglevel);
        } else {
            BLog_SetChannelLoglevel(i, DEFAULT_LOGLEVEL);
        }
    }
    
    BLog(BLOG_NOTICE, "initializing "GLOBAL_PRODUCT_NAME" "PROGRAM_NAME" "GLOBAL_VERSION);
    
    // initialize network
    if (!BNetwork_GlobalInit()) {
        BLog(BLOG_ERROR, "BNetwork_GlobalInit failed");
        goto fail1;
    }
    
    // init time
    BTime_Init();
    
    // init reactor
    if (!BReactor_Init(&reactor)) {
        BLog(BLOG_ERROR, "BReactor_Init failed");
        goto fail1;
    }
    
    // init process manager
    if (!BProcessManager_Init(&manager, &reactor)) {
        BLog(BLOG_ERROR, "BProcessManager_Init failed");
        goto fail2;
    }
    
    // init udev manager
    NCDUdevManager_Init(&umanager, options.no_udev, &reactor, &manager);
    
    // init random number generator
    if (!BRandom2_Init(&random2, BRANDOM2_INIT_LAZY)) {
        BLog(BLOG_ERROR, "BRandom2_Init failed");
        goto fail3;
    }
    
    // setup signal handler
    if (!BSignal_Init(&reactor, signal_handler, NULL)) {
        BLog(BLOG_ERROR, "BSignal_Init failed");
        goto fail4;
    }
    
    // build program
    NCDProgram program;
    if (!NCDBuildProgram_Build(options.config_file, &program)) {
        BLog(BLOG_ERROR, "failed to build program");
        goto fail5;
    }
    
    // setup interpreter parameters
    struct NCDInterpreter_params params;
    params.handler_finished = interpreter_handler_finished;
    params.user = NULL;
    params.retry_time = options.retry_time;
    params.extra_args = options.extra_args;
    params.num_extra_args = options.num_extra_args;
    params.reactor = &reactor;
    params.manager = &manager;
    params.umanager = &umanager;
    params.random2 = &random2;
    
    // initialize interpreter
    if (!NCDInterpreter_Init(&interpreter, program, params)) {
        goto fail5;
    }
    
    // don't enter event loop if syntax check is requested
    if (options.syntax_only) {
        main_exit_code = 0;
        goto fail6;
    }
    
    BLog(BLOG_NOTICE, "entering event loop");
    
    // enter event loop
    main_exit_code = BReactor_Exec(&reactor);
    
fail6:
    // free interpreter
    NCDInterpreter_Free(&interpreter);
fail5:
    // remove signal handler
    BSignal_Finish();
fail4:
    // free random number generator
    BRandom2_Free(&random2);
fail3:
    // free udev manager
    NCDUdevManager_Free(&umanager);
    
    // free process manager
    BProcessManager_Free(&manager);
fail2:
    // free reactor
    BReactor_Free(&reactor);
fail1:
    // free logger
    BLog(BLOG_NOTICE, "exiting");
    BLog_Free();
fail0:
    // finish objects
    DebugObjectGlobal_Finish();
    
    return main_exit_code;
}

void print_help (const char *name)
{
    printf(
        "Usage:\n"
        "    %s\n"
        "        [--help]\n"
        "        [--version]\n"
        "        [--logger <stdout/stderr/syslog>]\n"
        "        (logger=syslog?\n"
        "            [--syslog-facility <string>]\n"
        "            [--syslog-ident <string>]\n"
        "        )\n"
        "        [--loglevel <0-5/none/error/warning/notice/info/debug>]\n"
        "        [--channel-loglevel <channel-name> <0-5/none/error/warning/notice/info/debug>] ...\n"
        "        [--retry-time <ms>]\n"
        "        [--no-udev]\n"
        "        [--config-file <ncd_program_file>]\n"
        "        [--syntax-only]\n"
        "        [--signal-exit-code <number>]\n"
        "        [-- program_args...]\n"
        "        [<ncd_program_file> program_args...]\n" ,
        name
    );
}

void print_version (void)
{
    printf(GLOBAL_PRODUCT_NAME" "PROGRAM_NAME" "GLOBAL_VERSION"\n"GLOBAL_COPYRIGHT_NOTICE"\n");
}

int parse_arguments (int argc, char *argv[])
{
    if (argc <= 0) {
        return 0;
    }
    
    options.help = 0;
    options.version = 0;
    options.logger = LOGGER_STDERR;
#ifdef BADVPN_USE_SYSLOG
    options.logger_syslog_facility = "daemon";
    options.logger_syslog_ident = argv[0];
#endif
    options.loglevel = -1;
    for (int i = 0; i < BLOG_NUM_CHANNELS; i++) {
        options.loglevels[i] = -1;
    }
    options.config_file = NULL;
    options.syntax_only = 0;
    options.retry_time = DEFAULT_RETRY_TIME;
    options.signal_exit_code = DEFAULT_SIGNAL_EXIT_CODE;
    options.no_udev = 0;
    options.extra_args = NULL;
    options.num_extra_args = 0;
    
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (!strcmp(arg, "--help")) {
            options.help = 1;
        }
        else if (!strcmp(arg, "--version")) {
            options.version = 1;
        }
        else if (!strcmp(arg, "--logger")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            char *arg2 = argv[i + 1];
            if (!strcmp(arg2, "stdout")) {
                options.logger = LOGGER_STDOUT;
            }
            else if (!strcmp(arg2, "stderr")) {
                options.logger = LOGGER_STDERR;
            }
#ifdef BADVPN_USE_SYSLOG
            else if (!strcmp(arg2, "syslog")) {
                options.logger = LOGGER_SYSLOG;
            }
#endif
            else {
                fprintf(stderr, "%s: wrong argument\n", arg);
                return 0;
            }
            i++;
        }
#ifdef BADVPN_USE_SYSLOG
        else if (!strcmp(arg, "--syslog-facility")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            options.logger_syslog_facility = argv[i + 1];
            i++;
        }
        else if (!strcmp(arg, "--syslog-ident")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            options.logger_syslog_ident = argv[i + 1];
            i++;
        }
#endif
        else if (!strcmp(arg, "--loglevel")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            if ((options.loglevel = parse_loglevel(argv[i + 1])) < 0) {
                fprintf(stderr, "%s: wrong argument\n", arg);
                return 0;
            }
            i++;
        }
        else if (!strcmp(arg, "--channel-loglevel")) {
            if (2 >= argc - i) {
                fprintf(stderr, "%s: requires two arguments\n", arg);
                return 0;
            }
            int channel = BLogGlobal_GetChannelByName(argv[i + 1]);
            if (channel < 0) {
                fprintf(stderr, "%s: wrong channel argument\n", arg);
                return 0;
            }
            int loglevel = parse_loglevel(argv[i + 2]);
            if (loglevel < 0) {
                fprintf(stderr, "%s: wrong loglevel argument\n", arg);
                return 0;
            }
            options.loglevels[channel] = loglevel;
            i += 2;
        }
        else if (!strcmp(arg, "--config-file")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            options.config_file = argv[i + 1];
            i++;
        }
        else if (!strcmp(arg, "--syntax-only")) {
            options.syntax_only = 1;
        }
        else if (!strcmp(arg, "--retry-time")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            if ((options.retry_time = atoi(argv[i + 1])) < 0) {
                fprintf(stderr, "%s: wrong argument\n", arg);
                return 0;
            }
            i++;
        }
        else if (!strcmp(arg, "--signal-exit-code")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            if ((options.signal_exit_code = atoi(argv[i + 1])) < 0) {
                fprintf(stderr, "%s: wrong argument\n", arg);
                return 0;
            }
            i++;
        }
        else if (!strcmp(arg, "--no-udev")) {
            options.no_udev = 1;
        }
        else if (!strcmp(arg, "--")) {
            options.extra_args = &argv[i + 1];
            options.num_extra_args = argc - i - 1;
            i += options.num_extra_args;
        }
        else if (!string_begins_with(arg, "--")) {
            if (options.config_file) {
                fprintf(stderr, "%s: program is already specified (did you mean to use -- ?)\n", arg);
                return 0;
            }
            options.config_file = argv[i];
            options.extra_args = &argv[i + 1];
            options.num_extra_args = argc - i - 1;
            i += options.num_extra_args;
        }
        else {
            fprintf(stderr, "unknown option: %s\n", arg);
            return 0;
        }
    }
    
    if (options.help || options.version) {
        return 1;
    }
    
    if (!options.config_file) {
        fprintf(stderr, "No program is specified.\n");
        return 0;
    }
    
    return 1;
}

void signal_handler (void *unused)
{
    BLog(BLOG_NOTICE, "termination requested");
    
    NCDInterpreter_RequestShutdown(&interpreter, options.signal_exit_code);
}

void interpreter_handler_finished (void *user, int exit_code)
{
    BReactor_Quit(&reactor, exit_code);
}
