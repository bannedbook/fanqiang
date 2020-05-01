/**
 * @file flooder.c
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <protocol/addr.h>
#include <protocol/scproto.h>
#include <misc/loglevel.h>
#include <misc/version.h>
#include <misc/nsskey.h>
#include <misc/byteorder.h>
#include <misc/loggers_string.h>
#include <misc/open_standard_streams.h>
#include <base/BLog.h>
#include <system/BReactor.h>
#include <system/BSignal.h>
#include <system/BNetwork.h>
#include <flow/SinglePacketBuffer.h>
#include <flow/PacketProtoEncoder.h>
#include <nspr_support/BSSLConnection.h>
#include <server_connection/ServerConnection.h>

#ifndef BADVPN_USE_WINAPI
#include <base/BLog_syslog.h>
#endif

#include <flooder/flooder.h>

#include <generated/blog_channel_flooder.h>

#define LOGGER_STDOUT 1
#define LOGGER_SYSLOG 2

// command-line options
struct {
    int help;
    int version;
    int logger;
    #ifndef BADVPN_USE_WINAPI
    char *logger_syslog_facility;
    char *logger_syslog_ident;
    #endif
    int loglevel;
    int loglevels[BLOG_NUM_CHANNELS];
    int ssl;
    char *nssdb;
    char *client_cert_name;
    char *server_name;
    char *server_addr;
    peerid_t floods[MAX_FLOODS];
    int num_floods;
} options;

// server address we connect to
BAddr server_addr;

// server name to use for SSL
char server_name[256];

// reactor
BReactor ss;

// client certificate if using SSL
CERTCertificate *client_cert;

// client private key if using SSL
SECKEYPrivateKey *client_key;

// server connection
ServerConnection server;

// whether server is ready
int server_ready;

// my ID, defined only after server_ready
peerid_t my_id;

// flooding output
PacketRecvInterface flood_source;
PacketProtoEncoder flood_encoder;
SinglePacketBuffer flood_buffer;

// whether we were asked for a packet and blocked
int flood_blocking;

// index of next peer to send packet too
int flood_next;

/**
 * Cleans up everything that can be cleaned up from inside the event loop.
 */
static void terminate (void);

/**
 * Prints command line help.
 */
static void print_help (const char *name);

/**
 * Prints program name, version and copyright notice.
 */
static void print_version (void);

/**
 * Parses command line options into the options strucute.
 *
 * @return 1 on success, 0 on failure
 */
static int parse_arguments (int argc, char *argv[]);

/**
 * Processes command line options.
 *
 * @return 1 on success, 0 on failure
 */
static int resolve_arguments (void);

/**
 * Handler invoked when program termination is requested.
 */
static void signal_handler (void *unused);

static void server_handler_error (void *user);
static void server_handler_ready (void *user, peerid_t param_my_id, uint32_t ext_ip);
static void server_handler_newclient (void *user, peerid_t peer_id, int flags, const uint8_t *cert, int cert_len);
static void server_handler_endclient (void *user, peerid_t peer_id);
static void server_handler_message (void *user, peerid_t peer_id, uint8_t *data, int data_len);

static void flood_source_handler_recv (void *user, uint8_t *data);

int main (int argc, char *argv[])
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
    
    // initialize logger
    switch (options.logger) {
        case LOGGER_STDOUT:
            BLog_InitStdout();
            break;
        #ifndef BADVPN_USE_WINAPI
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
    
    // resolve addresses
    if (!resolve_arguments()) {
        BLog(BLOG_ERROR, "Failed to resolve arguments");
        goto fail1;
    }
    
    // init reactor
    if (!BReactor_Init(&ss)) {
        BLog(BLOG_ERROR, "BReactor_Init failed");
        goto fail1;
    }
    
    // setup signal handler
    if (!BSignal_Init(&ss, signal_handler, NULL)) {
        BLog(BLOG_ERROR, "BSignal_Init failed");
        goto fail1a;
    }
    
    if (options.ssl) {
        // init NSPR
        PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
        
        // register local NSPR file types
        if (!BSSLConnection_GlobalInit()) {
            BLog(BLOG_ERROR, "BSSLConnection_GlobalInit failed");
            goto fail3;
        }
        
        // init NSS
        if (NSS_Init(options.nssdb) != SECSuccess) {
            BLog(BLOG_ERROR, "NSS_Init failed (%d)", (int)PR_GetError());
            goto fail2;
        }
        
        // set cipher policy
        if (NSS_SetDomesticPolicy() != SECSuccess) {
            BLog(BLOG_ERROR, "NSS_SetDomesticPolicy failed (%d)", (int)PR_GetError());
            goto fail3;
        }
        
        // init server cache
        if (SSL_ConfigServerSessionIDCache(0, 0, 0, NULL) != SECSuccess) {
            BLog(BLOG_ERROR, "SSL_ConfigServerSessionIDCache failed (%d)", (int)PR_GetError());
            goto fail3;
        }
        
        // open server certificate and private key
        if (!open_nss_cert_and_key(options.client_cert_name, &client_cert, &client_key)) {
            BLog(BLOG_ERROR, "Cannot open certificate and key");
            goto fail4;
        }
    }
    
    // start connecting to server
    if (!ServerConnection_Init(
        &server, &ss, NULL, server_addr, SC_KEEPALIVE_INTERVAL, SERVER_BUFFER_MIN_PACKETS, options.ssl, 0, client_cert, client_key, server_name, NULL,
        server_handler_error, server_handler_ready, server_handler_newclient, server_handler_endclient, server_handler_message
    )) {
        BLog(BLOG_ERROR, "ServerConnection_Init failed");
        goto fail5;
    }
    
    // set server not ready
    server_ready = 0;
    
    // enter event loop
    BLog(BLOG_NOTICE, "entering event loop");
    BReactor_Exec(&ss);
    
    if (server_ready) {
        ServerConnection_ReleaseBuffers(&server);
        SinglePacketBuffer_Free(&flood_buffer);
        PacketProtoEncoder_Free(&flood_encoder);
        PacketRecvInterface_Free(&flood_source);
    }
    
    ServerConnection_Free(&server);
fail5:
    if (options.ssl) {
        CERT_DestroyCertificate(client_cert);
        SECKEY_DestroyPrivateKey(client_key);
fail4:
        ASSERT_FORCE(SSL_ShutdownServerSessionIDCache() == SECSuccess)
fail3:
        SSL_ClearSessionCache();
        ASSERT_FORCE(NSS_Shutdown() == SECSuccess)
fail2:
        ASSERT_FORCE(PR_Cleanup() == PR_SUCCESS)
        PL_ArenaFinish();
    }
    
    BSignal_Finish();
fail1a:
    BReactor_Free(&ss);
fail1:
    BLog(BLOG_NOTICE, "exiting");
    BLog_Free();
fail0:
    DebugObjectGlobal_Finish();
    
    return 1;
}

void terminate (void)
{
    BLog(BLOG_NOTICE, "tearing down");
    
    // exit event loop
    BReactor_Quit(&ss, 0);
}

void print_help (const char *name)
{
    printf(
        "Usage:\n"
        "    %s\n"
        "        [--help]\n"
        "        [--version]\n"
        "        [--logger <"LOGGERS_STRING">]\n"
        #ifndef BADVPN_USE_WINAPI
        "        (logger=syslog?\n"
        "            [--syslog-facility <string>]\n"
        "            [--syslog-ident <string>]\n"
        "        )\n"
        #endif
        "        [--loglevel <0-5/none/error/warning/notice/info/debug>]\n"
        "        [--channel-loglevel <channel-name> <0-5/none/error/warning/notice/info/debug>] ...\n"
        "        [--ssl --nssdb <string> --client-cert-name <string>]\n"
        "        [--server-name <string>]\n"
        "        --server-addr <addr>\n"
        "        [--flood-id <id>] ...\n"
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
    if (argc <= 0) {
        return 0;
    }
    
    options.help = 0;
    options.version = 0;
    options.logger = LOGGER_STDOUT;
    #ifndef BADVPN_USE_WINAPI
    options.logger_syslog_facility = "daemon";
    options.logger_syslog_ident = argv[0];
    #endif
    options.loglevel = -1;
    for (int i = 0; i < BLOG_NUM_CHANNELS; i++) {
        options.loglevels[i] = -1;
    }
    options.ssl = 0;
    options.nssdb = NULL;
    options.client_cert_name = NULL;
    options.server_name = NULL;
    options.server_addr = NULL;
    options.num_floods = 0;
    
    int i;
    for (i = 1; i < argc; i++) {
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
            #ifndef BADVPN_USE_WINAPI
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
        #ifndef BADVPN_USE_WINAPI
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
        else if (!strcmp(arg, "--ssl")) {
            options.ssl = 1;
        }
        else if (!strcmp(arg, "--nssdb")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            options.nssdb = argv[i + 1];
            i++;
        }
        else if (!strcmp(arg, "--client-cert-name")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            options.client_cert_name = argv[i + 1];
            i++;
        }
        else if (!strcmp(arg, "--server-name")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            options.server_name = argv[i + 1];
            i++;
        }
        else if (!strcmp(arg, "--server-addr")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            options.server_addr = argv[i + 1];
            i++;
        }
        else if (!strcmp(arg, "--flood-id")) {
            if (1 >= argc - i) {
                fprintf(stderr, "%s: requires an argument\n", arg);
                return 0;
            }
            if (options.num_floods == MAX_FLOODS) {
                fprintf(stderr, "%s: too many\n", arg);
                return 0;
            }
            options.floods[options.num_floods] = atoi(argv[i + 1]);
            options.num_floods++;
            i++;
        }
        else {
            fprintf(stderr, "unknown option: %s\n", arg);
            return 0;
        }
    }
    
    if (options.help || options.version) {
        return 1;
    }
    
    if (options.ssl != !!options.nssdb) {
        fprintf(stderr, "False: --ssl <=> --nssdb\n");
        return 0;
    }
    
    if (options.ssl != !!options.client_cert_name) {
        fprintf(stderr, "False: --ssl <=> --client-cert-name\n");
        return 0;
    }
    
    if (!options.server_addr) {
        fprintf(stderr, "False: --server-addr\n");
        return 0;
    }
    
    return 1;
}

int resolve_arguments (void)
{
    // resolve server address
    ASSERT(options.server_addr)
    if (!BAddr_Parse(&server_addr, options.server_addr, server_name, sizeof(server_name))) {
        BLog(BLOG_ERROR, "server addr: BAddr_Parse failed");
        return 0;
    }
    if (!addr_supported(server_addr)) {
        BLog(BLOG_ERROR, "server addr: not supported");
        return 0;
    }
    
    // override server name if requested
    if (options.server_name) {
        if (strlen(options.server_name) >= sizeof(server_name)) {
            BLog(BLOG_ERROR, "server name: too long");
            return 0;
        }
        strcpy(server_name, options.server_name);
    }
    
    return 1;
}

void signal_handler (void *unused)
{
    BLog(BLOG_NOTICE, "termination requested");
    
    terminate();
}

void server_handler_error (void *user)
{
    BLog(BLOG_ERROR, "server connection failed, exiting");
    
    terminate();
}

void server_handler_ready (void *user, peerid_t param_my_id, uint32_t ext_ip)
{
    ASSERT(!server_ready)
    
    // remember our ID
    my_id = param_my_id;
    
    // init flooding
    
    // init source
    PacketRecvInterface_Init(&flood_source, SC_MAX_ENC, flood_source_handler_recv, NULL, BReactor_PendingGroup(&ss));
    
    // init encoder
    PacketProtoEncoder_Init(&flood_encoder, &flood_source, BReactor_PendingGroup(&ss));
    
    // init buffer
    if (!SinglePacketBuffer_Init(&flood_buffer, PacketProtoEncoder_GetOutput(&flood_encoder), ServerConnection_GetSendInterface(&server), BReactor_PendingGroup(&ss))) {
        BLog(BLOG_ERROR, "SinglePacketBuffer_Init failed, exiting");
        goto fail1;
    }
    
    // set not blocking
    flood_blocking = 0;
    
    // set server ready
    server_ready = 1;
    
    BLog(BLOG_INFO, "server: ready, my ID is %d", (int)my_id);
    
    return;
    
fail1:
    PacketProtoEncoder_Free(&flood_encoder);
    PacketRecvInterface_Free(&flood_source);
    terminate();
}

void server_handler_newclient (void *user, peerid_t peer_id, int flags, const uint8_t *cert, int cert_len)
{
    ASSERT(server_ready)
    
    BLog(BLOG_INFO, "newclient %d", (int)peer_id);
}

void server_handler_endclient (void *user, peerid_t peer_id)
{
    ASSERT(server_ready)
    
    BLog(BLOG_INFO, "endclient %d", (int)peer_id);
}

void server_handler_message (void *user, peerid_t peer_id, uint8_t *data, int data_len)
{
    ASSERT(server_ready)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= SC_MAX_MSGLEN)
    
    BLog(BLOG_INFO, "message from %d", (int)peer_id);
}

void flood_source_handler_recv (void *user, uint8_t *data)
{
    ASSERT(server_ready)
    ASSERT(!flood_blocking)
    if (options.num_floods > 0) {
        ASSERT(flood_next >= 0)
        ASSERT(flood_next < options.num_floods)
    }
    
    if (options.num_floods == 0) {
        flood_blocking = 1;
        return;
    }
    
    peerid_t peer_id = options.floods[flood_next];
    flood_next = (flood_next + 1) % options.num_floods;
    
    BLog(BLOG_INFO, "message to %d", (int)peer_id);
    
    struct sc_header header;
    header.type = SCID_OUTMSG;
    memcpy(data, &header, sizeof(header));
    
    struct sc_client_outmsg omsg;
    omsg.clientid = htol16(peer_id);
    memcpy(data + sizeof(header), &omsg, sizeof(omsg));
    
    memset(data + sizeof(struct sc_header) + sizeof(struct sc_client_outmsg), 0, SC_MAX_MSGLEN);
    
    PacketRecvInterface_Done(&flood_source, sizeof(struct sc_header) + sizeof(struct sc_client_outmsg) + SC_MAX_MSGLEN);
}
