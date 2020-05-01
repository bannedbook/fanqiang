/**
 * @file dostest-attacker.c
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

#include <misc/debug.h>
#include <misc/version.h>
#include <misc/offset.h>
#include <misc/open_standard_streams.h>
#include <misc/balloc.h>
#include <misc/loglevel.h>
#include <misc/minmax.h>
#include <structure/LinkedList1.h>
#include <base/BLog.h>
#include <system/BAddr.h>
#include <system/BReactor.h>
#include <system/BNetwork.h>
#include <system/BConnection.h>
#include <system/BSignal.h>

#include <generated/blog_channel_dostest_attacker.h>

#define PROGRAM_NAME "dostest-attacker"

// connection structure
struct connection {
    int connected;
    BConnector connector;
    BConnection con;
    StreamRecvInterface *recv_if;
    uint8_t buf[512];
    LinkedList1Node connections_list_node;
};

// command-line options
static struct {
    int help;
    int version;
    char *connect_addr;
    int max_connections;
    int max_connecting;
    int loglevel;
    int loglevels[BLOG_NUM_CHANNELS];
} options;

// connect address
static BAddr connect_addr;

// reactor
static BReactor reactor;

// connections
static LinkedList1 connections_list;
static int num_connections;
static int num_connecting;

// timer for scheduling creation of more connections
static BTimer make_connections_timer;

static void print_help (const char *name);
static void print_version (void);
static int parse_arguments (int argc, char *argv[]);
static int process_arguments (void);
static void signal_handler (void *unused);
static int connection_new (void);
static void connection_free (struct connection *conn);
static void connection_logfunc (struct connection *conn);
static void connection_log (struct connection *conn, int level, const char *fmt, ...);
static void connection_connector_handler (struct connection *conn, int is_error);
static void connection_connection_handler (struct connection *conn, int event);
static void connection_recv_handler_done (struct connection *conn, int data_len);
static void make_connections_timer_handler (void *unused);

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
    if (!BReactor_Init(&reactor)) {
        BLog(BLOG_ERROR, "BReactor_Init failed");
        goto fail1;
    }
    
    // setup signal handler
    if (!BSignal_Init(&reactor, signal_handler, NULL)) {
        BLog(BLOG_ERROR, "BSignal_Init failed");
        goto fail2;
    }
    
    // init connections list
    LinkedList1_Init(&connections_list);
    num_connections = 0;
    num_connecting = 0;
    
    // init make connections timer
    BTimer_Init(&make_connections_timer, 0, make_connections_timer_handler, NULL);
    BReactor_SetTimer(&reactor, &make_connections_timer);
    
    // enter event loop
    BLog(BLOG_NOTICE, "entering event loop");
    BReactor_Exec(&reactor);
    
    // free connections
    while (!LinkedList1_IsEmpty(&connections_list)) {
        struct connection *conn = UPPER_OBJECT(LinkedList1_GetFirst(&connections_list), struct connection, connections_list_node);
        connection_free(conn);
    }
    // free make connections timer
    BReactor_RemoveTimer(&reactor, &make_connections_timer);
    // free signal
    BSignal_Finish();
fail2:
    // free reactor
    BReactor_Free(&reactor);
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
        "        --connect-addr <addr>\n"
        "        --max-connections <number>\n"
        "        --max-connecting <number>\n"
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
    options.connect_addr = NULL;
    options.max_connections = -1;
    options.max_connecting = -1;
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
        else if (!strcmp(arg, "--connect-addr")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            options.connect_addr = argv[i + 1];
            i++;
        }
        else if (!strcmp(arg, "--max-connections")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            if ((options.max_connections = atoi(argv[i + 1])) <= 0) {
                fprintf(stderr, "%s: wrong argument\n", arg);
                return 0;
            }
            i++;
        }
        else if (!strcmp(arg, "--max-connecting")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            if ((options.max_connecting = atoi(argv[i + 1])) <= 0) {
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
    
    if (!options.connect_addr) {
        fprintf(stderr, "--connect-addr missing\n");
        return 0;
    }
    
    if (options.max_connections == -1) {
        fprintf(stderr, "--max-connections missing\n");
        return 0;
    }
    
    if (options.max_connecting == -1) {
        fprintf(stderr, "--max-connecting missing\n");
        return 0;
    }
    
    return 1;
}

int process_arguments (void)
{
    // resolve listen address
    if (!BAddr_Parse(&connect_addr, options.connect_addr, NULL, 0)) {
        BLog(BLOG_ERROR, "connect addr: BAddr_Parse failed");
        return 0;
    }
    
    return 1;
}

void signal_handler (void *unused)
{
    BLog(BLOG_NOTICE, "termination requested");
    
    // exit event loop
    BReactor_Quit(&reactor, 1);
}

int connection_new (void)
{
    // allocate structure
    struct connection *conn = (struct connection *)malloc(sizeof(*conn));
    if (!conn) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // set not connected
    conn->connected = 0;
    
    // init connector
    if (!BConnector_Init(&conn->connector, connect_addr, &reactor, conn, (BConnector_handler)connection_connector_handler)) {
        BLog(BLOG_ERROR, "BConnector_Init failed");
        goto fail1;
    }
    
    // add to connections list
    LinkedList1_Append(&connections_list, &conn->connections_list_node);
    num_connections++;
    num_connecting++;
    
    return 1;
    
fail1:
    free(conn);
fail0:
    return 0;
}

void connection_free (struct connection *conn)
{
    // remove from connections list
    LinkedList1_Remove(&connections_list, &conn->connections_list_node);
    num_connections--;
    if (!conn->connected) {
        num_connecting--;
    }
    
    if (conn->connected) {
        // free receive interface
        BConnection_RecvAsync_Free(&conn->con);
        
        // free connection
        BConnection_Free(&conn->con);
    }
    
    // free connector
    BConnector_Free(&conn->connector);
    
    // free structure
    free(conn);
}

void connection_logfunc (struct connection *conn)
{
    BLog_Append("%d connection (%p): ", num_connecting, (void *)conn);
}

void connection_log (struct connection *conn, int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    BLog_LogViaFuncVarArg((BLog_logfunc)connection_logfunc, conn, BLOG_CURRENT_CHANNEL, level, fmt, vl);
    va_end(vl);
}

void connection_connector_handler (struct connection *conn, int is_error)
{
    ASSERT(!conn->connected)
    
    // check for connection error
    if (is_error) {
        connection_log(conn, BLOG_INFO, "failed to connect");
        goto fail0;
    }
    
    // init connection from connector
    if (!BConnection_Init(&conn->con, BConnection_source_connector(&conn->connector), &reactor, conn, (BConnection_handler)connection_connection_handler)) {
        connection_log(conn, BLOG_INFO, "BConnection_Init failed");
        goto fail0;
    }
    
    // init receive interface
    BConnection_RecvAsync_Init(&conn->con);
    conn->recv_if = BConnection_RecvAsync_GetIf(&conn->con);
    StreamRecvInterface_Receiver_Init(conn->recv_if, (StreamRecvInterface_handler_done)connection_recv_handler_done, conn);
    
    // start receiving
    StreamRecvInterface_Receiver_Recv(conn->recv_if, conn->buf, sizeof(conn->buf));
    
    // no longer connecting
    conn->connected = 1;
    num_connecting--;
    
    connection_log(conn, BLOG_INFO, "connected");
    
    // schedule making connections (because of connecting limit)
    BReactor_SetTimer(&reactor, &make_connections_timer);
    return;
    
fail0:
    // free connection
    connection_free(conn);
    
    // schedule making connections
    BReactor_SetTimer(&reactor, &make_connections_timer);
}

void connection_connection_handler (struct connection *conn, int event)
{
    ASSERT(conn->connected)
    
    if (event == BCONNECTION_EVENT_RECVCLOSED) {
        connection_log(conn, BLOG_INFO, "connection closed");
    } else {
        connection_log(conn, BLOG_INFO, "connection error");
    }
    
    // free connection
    connection_free(conn);
    
    // schedule making connections
    BReactor_SetTimer(&reactor, &make_connections_timer);
}

void connection_recv_handler_done (struct connection *conn, int data_len)
{
    ASSERT(conn->connected)
    
    // receive more
    StreamRecvInterface_Receiver_Recv(conn->recv_if, conn->buf, sizeof(conn->buf));
    
    connection_log(conn, BLOG_INFO, "received %d bytes", data_len);
}

void make_connections_timer_handler (void *unused)
{
    int make_num = bmin_int(options.max_connections - num_connections, options.max_connecting - num_connecting);
    
    if (make_num <= 0) {
        return;
    }
    
    BLog(BLOG_INFO, "making %d connections", make_num);
    
    for (int i = 0; i < make_num; i++) {
        if (!connection_new()) {
            // can happen if fd limit is reached
            BLog(BLOG_ERROR, "failed to make connection, waiting");
            BReactor_SetTimerAfter(&reactor, &make_connections_timer, 10);
            return;
        }
    }
}
