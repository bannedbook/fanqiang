/**
 * @file dostest-server.c
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef BADVPN_LINUX
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include <misc/debug.h>
#include <misc/version.h>
#include <misc/offset.h>
#include <misc/open_standard_streams.h>
#include <misc/balloc.h>
#include <misc/loglevel.h>
#include <structure/LinkedList1.h>
#include <base/BLog.h>
#include <system/BAddr.h>
#include <system/BReactor.h>
#include <system/BNetwork.h>
#include <system/BConnection.h>
#include <system/BSignal.h>
#include "StreamBuffer.h"

#include <generated/blog_channel_dostest_server.h>

#define PROGRAM_NAME "dostest-server"

#ifdef BADVPN_LINUX
#ifndef SO_DOSDEF_PREPARE
#define SO_DOSDEF_PREPARE 48
#endif
#ifndef SO_DOSDEF_ACTIVATE
#define SO_DOSDEF_ACTIVATE 49
#endif
#endif

#define BUF_SIZE 1024

// client structure
struct client {
    BConnection con;
    BAddr addr;
    StreamBuffer buf;
    BTimer disconnect_timer;
    LinkedList1Node clients_list_node;
};

// command-line options
static struct {
    int help;
    int version;
    char *listen_addr;
    int max_clients;
    int disconnect_time;
    int defense_prepare_clients;
    int defense_activate_clients;
    int loglevel;
    int loglevels[BLOG_NUM_CHANNELS];
} options;

// listen address
static BAddr listen_addr;

// reactor
static BReactor ss;

// listener
static BListener listener;

// clients
static LinkedList1 clients_list;
static int num_clients;

// defense status
static int defense_prepare;
static int defense_activate;

static void print_help (const char *name);
static void print_version (void);
static int parse_arguments (int argc, char *argv[]);
static int process_arguments (void);
static void signal_handler (void *unused);
static void listener_handler (void *unused);
static void client_free (struct client *client);
static void client_logfunc (struct client *client);
static void client_log (struct client *client, int level, const char *fmt, ...);
static void client_disconnect_timer_handler (struct client *client);
static void client_connection_handler (struct client *client, int event);
static void update_defense (void);

int main (int argc, char **argv)
{
    if (argc <= 0) {
        return 1;
    }
    
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
    
    // init loger
    BLog_InitStderr();
    
    // configure logger channels
    for (int i = 0; i < BLOG_NUM_CHANNELS; i++) {
        if (options.loglevels[i] >= 0) {
            BLog_SetChannelLoglevel(i, options.loglevels[i]);
        }
        else if (options.loglevel >= 0) {
            BLog_SetChannelLoglevel(i, options.loglevel);
        }
    }
    
    BLog(BLOG_NOTICE, "initializing "GLOBAL_PRODUCT_NAME" "PROGRAM_NAME" "GLOBAL_VERSION);
    
    // initialize network
    if (!BNetwork_GlobalInit()) {
        BLog(BLOG_ERROR, "BNetwork_GlobalInit failed");
        goto fail1;
    }
    
    // process arguments
    if (!process_arguments()) {
        BLog(BLOG_ERROR, "Failed to process arguments");
        goto fail1;
    }
    
    // init time
    BTime_Init();
    
    // init reactor
    if (!BReactor_Init(&ss)) {
        BLog(BLOG_ERROR, "BReactor_Init failed");
        goto fail1;
    }
    
    // setup signal handler
    if (!BSignal_Init(&ss, signal_handler, NULL)) {
        BLog(BLOG_ERROR, "BSignal_Init failed");
        goto fail2;
    }
    
    // initialize listener
    if (!BListener_Init(&listener, listen_addr, &ss, NULL, listener_handler)) {
        BLog(BLOG_ERROR, "Listener_Init failed");
        goto fail3;
    }
    
    // init clients list
    LinkedList1_Init(&clients_list);
    num_clients = 0;
    
    // clear defense state
    defense_prepare = 0;
    defense_activate = 0;
    
    // update defense
    update_defense();
    
    // enter event loop
    BLog(BLOG_NOTICE, "entering event loop");
    BReactor_Exec(&ss);
    
    // free clients
    while (!LinkedList1_IsEmpty(&clients_list)) {
        struct client *client = UPPER_OBJECT(LinkedList1_GetFirst(&clients_list), struct client, clients_list_node);
        client_free(client);
    }
    // free listener
    BListener_Free(&listener);
fail3:
    // free signal
    BSignal_Finish();
fail2:
    // free reactor
    BReactor_Free(&ss);
fail1:
    // free logger
    BLog(BLOG_NOTICE, "exiting");
    BLog_Free();
fail0:
    // finish debug objects
    DebugObjectGlobal_Finish();
    
    return 1;
}

void print_help (const char *name)
{
    printf(
        "Usage:\n"
        "    %s\n"
        "        [--help]\n"
        "        [--version]\n"
        "        --listen-addr <addr>\n"
        "        --max-clients <number>\n"
        "        --disconnect-time <milliseconds>\n"
        "        [--defense-prepare-clients <number>]\n"
        "        [--defense-activate-clients <number>]\n"
        "        [--loglevel <0-5/none/error/warning/notice/info/debug>]\n"
        "        [--channel-loglevel <channel-name> <0-5/none/error/warning/notice/info/debug>] ...\n"
        "Address format is a.b.c.d:port (IPv4) or [addr]:port (IPv6).\n",
        name
    );
}

void print_version (void)
{
    printf(GLOBAL_PRODUCT_NAME" "PROGRAM_NAME" "GLOBAL_VERSION"\n"GLOBAL_COPYRIGHT_NOTICE"\n");
}

int parse_arguments (int argc, char *argv[])
{
    options.help = 0;
    options.version = 0;
    options.listen_addr = NULL;
    options.max_clients = -1;
    options.disconnect_time = -1;
    options.defense_prepare_clients = -1;
    options.defense_activate_clients = -1;
    options.loglevel = -1;
    for (int i = 0; i < BLOG_NUM_CHANNELS; i++) {
        options.loglevels[i] = -1;
    }
    
    int i;
    for (i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (!strcmp(arg, "--help")) {
            options.help = 1;
        }
        else if (!strcmp(arg, "--version")) {
            options.version = 1;
        }
        else if (!strcmp(arg, "--listen-addr")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            options.listen_addr = argv[i + 1];
            i++;
        }
        else if (!strcmp(arg, "--max-clients")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            if ((options.max_clients = atoi(argv[i + 1])) <= 0) {
                fprintf(stderr, "%s: wrong argument\n", arg);
                return 0;
            }
            i++;
        }
        else if (!strcmp(arg, "--disconnect-time")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            if ((options.disconnect_time = atoi(argv[i + 1])) <= 0) {
                fprintf(stderr, "%s: wrong argument\n", arg);
                return 0;
            }
            i++;
        }
        else if (!strcmp(arg, "--defense-prepare-clients")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            if ((options.defense_prepare_clients = atoi(argv[i + 1])) <= 0) {
                fprintf(stderr, "%s: wrong argument\n", arg);
                return 0;
            }
            i++;
        }
        else if (!strcmp(arg, "--defense-activate-clients")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            if ((options.defense_activate_clients = atoi(argv[i + 1])) <= 0) {
                fprintf(stderr, "%s: wrong argument\n", arg);
                return 0;
            }
            i++;
        }
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
        else {
            fprintf(stderr, "unknown option: %s\n", arg);
            return 0;
        }
    }
    
    if (options.help || options.version) {
        return 1;
    }
    
    if (!options.listen_addr) {
        fprintf(stderr, "--listen-addr missing\n");
        return 0;
    }
    
    if (options.max_clients == -1) {
        fprintf(stderr, "--max-clients missing\n");
        return 0;
    }
    
    if (options.disconnect_time == -1) {
        fprintf(stderr, "--disconnect-time missing\n");
        return 0;
    }
    
    return 1;
}

int process_arguments (void)
{
    // resolve listen address
    if (!BAddr_Parse(&listen_addr, options.listen_addr, NULL, 0)) {
        BLog(BLOG_ERROR, "listen addr: BAddr_Parse failed");
        return 0;
    }
    
    return 1;
}

void signal_handler (void *unused)
{
    BLog(BLOG_NOTICE, "termination requested");
    
    // exit event loop
    BReactor_Quit(&ss, 1);
}

void listener_handler (void *unused)
{
    if (num_clients == options.max_clients) {
        BLog(BLOG_ERROR, "maximum number of clients reached");
        goto fail0;
    }
    
    // allocate structure
    struct client *client = (struct client *)malloc(sizeof(*client));
    if (!client) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // accept client
    if (!BConnection_Init(&client->con, BConnection_source_listener(&listener, &client->addr), &ss, client, (BConnection_handler)client_connection_handler)) {
        BLog(BLOG_ERROR, "BConnection_Init failed");
        goto fail1;
    }
    
    // init connection interfaces
    BConnection_RecvAsync_Init(&client->con);
    BConnection_SendAsync_Init(&client->con);
    StreamRecvInterface *recv_if = BConnection_RecvAsync_GetIf(&client->con);
    StreamPassInterface *send_if = BConnection_SendAsync_GetIf(&client->con);
    
    // init stream buffer (to loop received data back to the client)
    if (!StreamBuffer_Init(&client->buf, BUF_SIZE, recv_if, send_if)) {
        BLog(BLOG_ERROR, "StreamBuffer_Init failed");
        goto fail2;
    }
    
    // init disconnect timer
    BTimer_Init(&client->disconnect_timer, options.disconnect_time, (BTimer_handler)client_disconnect_timer_handler, client);
    BReactor_SetTimer(&ss, &client->disconnect_timer);
    
    // insert to clients list
    LinkedList1_Append(&clients_list, &client->clients_list_node);
    num_clients++;
    
    client_log(client, BLOG_INFO, "connected");
    BLog(BLOG_NOTICE, "%d clients", num_clients);
    
    // update defense
    update_defense();
    return;
    
fail2:
    BConnection_SendAsync_Free(&client->con);
    BConnection_RecvAsync_Free(&client->con);
    BConnection_Free(&client->con);
fail1:
    free(client);
fail0:
    return;
}

void client_free (struct client *client)
{
    // remove from clients list
    LinkedList1_Remove(&clients_list, &client->clients_list_node);
    num_clients--;
    
    // free disconnect timer
    BReactor_RemoveTimer(&ss, &client->disconnect_timer);
    
    // free stream buffer
    StreamBuffer_Free(&client->buf);
    
    // free connection interfaces
    BConnection_SendAsync_Free(&client->con);
    BConnection_RecvAsync_Free(&client->con);
    
    // free connection
    BConnection_Free(&client->con);
    
    // free structure
    free(client);
    
    BLog(BLOG_NOTICE, "%d clients", num_clients);
    
    // update defense
    update_defense();
}

void client_logfunc (struct client *client)
{
    char addr[BADDR_MAX_PRINT_LEN];
    BAddr_Print(&client->addr, addr);
    
    BLog_Append("client (%s): ", addr);
}

void client_log (struct client *client, int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    BLog_LogViaFuncVarArg((BLog_logfunc)client_logfunc, client, BLOG_CURRENT_CHANNEL, level, fmt, vl);
    va_end(vl);
}

void client_disconnect_timer_handler (struct client *client)
{
    client_log(client, BLOG_INFO, "timed out, disconnecting");
    
    // free client
    client_free(client);
}

void client_connection_handler (struct client *client, int event)
{
    if (event == BCONNECTION_EVENT_RECVCLOSED) {
        client_log(client, BLOG_INFO, "client closed");
    } else {
        client_log(client, BLOG_INFO, "client error");
    }
    
    // free client
    client_free(client);
}

void update_defense (void)
{
#ifdef BADVPN_LINUX
    if (options.defense_prepare_clients != -1) {
        int val = num_clients >= options.defense_prepare_clients;
        int res = setsockopt(listener.fd, SOL_SOCKET, SO_DOSDEF_PREPARE, &val, sizeof(val));
        if (res < 0) {
            BLog(BLOG_ERROR, "failed to %s defense preparation", (val ? "enable" : "disable"));
        } else {
            if (!defense_prepare && val) {
                BLog(BLOG_NOTICE, "defense preparation enabled");
            }
            else if (defense_prepare && !val) {
                BLog(BLOG_NOTICE, "defense preparation disabled");
            }
        }
        defense_prepare = val;
    }
    
    if (options.defense_activate_clients != -1) {
        int val = num_clients >= options.defense_activate_clients;
        int res = setsockopt(listener.fd, SOL_SOCKET, SO_DOSDEF_ACTIVATE, &val, sizeof(val));
        if (res < 0) {
            BLog(BLOG_ERROR, "failed to %s defense activation", (val ? "enable" : "disable"));
        } else {
            if (!defense_activate && val) {
                BLog(BLOG_NOTICE, "defense activation enabled");
            }
            else if (defense_activate && !val) {
                BLog(BLOG_NOTICE, "defense activation disabled");
            }
        }
        defense_activate = val;
    }
#endif
}
