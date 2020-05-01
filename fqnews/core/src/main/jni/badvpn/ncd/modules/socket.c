/**
 * @file socket.c
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
 * Synopsis:
 *   sys.socket sys.connect(string addr [, map options])
 * 
 * Options:
 *   "read_size" - the maximum number of bytes that can be read by a single
 *     read() call. Must be greater than zero. Greater values may improve
 *     performance, but will increase memory usage. Default: 8192.
 * 
 * Variables:
 *   string is_error - "true" if there was an error with the connection,
 *     "false" if not
 * 
 * Description:
 *   Attempts to establish a connection to a server. The address should be
 *   in one of the following forms:
 *   - {"tcp", {"ipv4", ipv4_address, port_number}},
 *   - {"tcp", {"ipv6", ipv6_address, port_number}},
 *   - {"unix", socket_path},
 *   - {"device", device_path}.
 *   When the connection attempt is finished, the sys.connect() statement goes
 *   up, and the 'is_error' variable should be used to check for connection
 *   failure. If there was no error, the read(), write() and close() methods
 *   can be used to work with the connection.
 *   If an error occurs after the connection has been established, the
 *   sys.connect() statement will automatically trigger backtracking, and the
 *   'is_error' variable will be changed to "true". This means that all
 *   errors with the connection can be handled at the place of sys.connect(),
 *   and no special care is normally needed to handle error in read() and
 *   write().
 *   The special "device" address type may be used to connect to a serial
 *   port. But you need to configure the port yourself first (stty).
 *   WARNING: when you're not trying to either send or receive data, the
 *   connection may be unable to detect any events with the connection.
 *   You should never be neither sending nor receiving for an indefinite time.
 * 
 * Synopsis:
 *   sys.socket::read()
 * 
 * Variables:
 *   string (empty) - some data received from the socket, or empty on EOF
 *   string eof - "true" if EOF was encountered, "false" if not
 *   string not_eof - (deprecated) "true" if EOF was not encountered,
 *     "false" if it was
 * 
 * Description:
 *   Receives data from the connection. If EOF was encountered (remote host
 *   has closed the connection), this returns no data. Otherwise it returns
 *   at least one byte.
 *   WARNING: after you receive EOF from a sys.listen() type socket, is is
 *   your responsibility to call close() eventually, else the cline process
 *   may remain alive indefinitely.
 *   WARNING: this may return an arbitrarily small chunk of data. There is
 *   no significance to the size of the chunks. Correct code will behave
 *   the same no matter how the incoming data stream is split up.
 *   WARNING: if a read() is terminated while it is still in progress, i.e.
 *   has not gone up yet, then the connection is automatically closed, as
 *   if close() was called.
 * 
 * Synopsis:
 *   sys.socket::write(string data)
 * 
 * Description:
 *   Sends data to the connection.
 *   WARNING: this may block if the operating system's internal send buffer
 *   is full. Be careful not to enter a deadlock where both ends of the
 *   connection are trying to send data to the other, but neither is trying
 *   to receive any data.
 *   WARNING: if a write() is terminated while it is still in progress, i.e.
 *   has not gone up yet, then the connection is automatically closed, as
 *   if close() was called.
 * 
 * Synopsis:
 *   sys.socket::close()
 * 
 * Description:
 *   Closes the connection. After this, any further read(), write() or close()
 *   will trigger an error with the interpreter. For client sockets created
 *   via sys.listen(), this will immediately trigger termination of the client
 *   process.
 * 
 * Synopsis:
 *   sys.listen(string address, string client_template, list args [, map options])
 * 
 * Options:
 *   "read_size" - the maximum number of bytes that can be read by a single
 *     read() call. Must be greater than zero. Greater values may improve
 *     performance, but will increase memory usage. Default: 8192.
 * 
 * Variables:
 *   string is_error - "true" if listening failed to inittialize, "false" if
 *     not
 * 
 * Special objects and variables in client_template:
 *   sys.socket _socket - the socket object for the client
 *   string _socket.client_addr - the address of the client. The form is
 *     like the second part of the sys.connect() address format, e.g.
 *     {"ipv4", "1.2.3.4", "4000"}.
 * 
 * Description:
 *   Starts listening on the specified address. The format of the device is
 *   as described for sys.connect, but without the "device" address type.
 *   The 'is_error' variable
 *   reflects the success of listening initiation. If listening succeeds,
 *   then for every client that connects, a process is automatically created
 *   from the template specified by 'client_template', and the 'args' list
 *   is used as template arguments. Inside such processes, a special object
 *   '_socket' is available, which represents the connection, and supports
 *   the same methods as sys.connect(), i.e. read(), write() and close().
 *   When an error occurs with the connection, the socket is automatically
 *   closed, triggering process termination.
 */

#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <structure/LinkedList0.h>
#include <system/BConnection.h>
#include <system/BConnectionGeneric.h>
#include <ncd/extra/address_utils.h>
#include <ncd/extra/NCDBuf.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_socket.h>

#define CONNECTION_TYPE_CONNECT 1
#define CONNECTION_TYPE_LISTEN 2

#define CONNECTION_STATE_CONNECTING 1
#define CONNECTION_STATE_ESTABLISHED 2
#define CONNECTION_STATE_ERROR 3
#define CONNECTION_STATE_ABORTED 4

#define DEFAULT_READ_BUF_SIZE 8192

struct connection {
    union {
        struct {
            NCDModuleInst *i;
            BConnector connector;
            size_t read_buf_size;
        } connect;
        struct {
            struct listen_instance *listen_inst;
            LinkedList0Node clients_list_node;
            BAddr addr;
            NCDModuleProcess process;
        } listen;
    };
    
    unsigned int type:2;
    unsigned int state:3;
    unsigned int recv_closed:1;
    BConnection connection;
    NCDBufStore store;
    struct read_instance *read_inst;
    struct write_instance *write_inst;
};

struct read_instance {
    NCDModuleInst *i;
    struct connection *con_inst;
    NCDBuf *buf;
    size_t read_size;
};

struct write_instance {
    NCDModuleInst *i;
    struct connection *con_inst;
    MemRef data;
};

struct listen_instance {
    NCDModuleInst *i;
    unsigned int have_error:1;
    unsigned int dying:1;
    size_t read_buf_size;
    NCDValRef client_template;
    NCDValRef client_template_args;
    BListener listener;
    LinkedList0 clients_list;
};

enum {STRING_SOCKET, STRING_SYS_SOCKET, STRING_CLIENT_ADDR};

static const char *strings[] = {
    "_socket", "sys.socket", "client_addr", NULL
};

static int parse_options (NCDModuleInst *i, NCDValRef options, size_t *out_read_size);
static void connection_log (struct connection *o, int level, const char *fmt, ...);
static void connection_free_connection (struct connection *o);
static void connection_error (struct connection *o);
static void connection_abort (struct connection *o);
static void connection_connector_handler (void *user, int is_error);
static void connection_complete_establish (struct connection *o);
static void connection_connection_handler (void *user, int event);
static void connection_send_handler_done (void *user, int data_len);
static void connection_recv_handler_done (void *user, int data_len);
static void connection_process_handler (struct NCDModuleProcess_s *process, int event);
static int connection_process_func_getspecialobj (struct NCDModuleProcess_s *process, NCD_string_id_t name, NCDObject *out_object);
static int connection_process_socket_obj_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value);
static int connection_process_caller_obj_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object);
static void listen_listener_handler (void *user);

static int parse_options (NCDModuleInst *i, NCDValRef options, size_t *out_read_size)
{
    ASSERT(out_read_size)
    
    *out_read_size = DEFAULT_READ_BUF_SIZE;
    
    if (!NCDVal_IsInvalid(options)) {
        if (!NCDVal_IsMap(options)) {
            ModuleLog(i, BLOG_ERROR, "options argument is not a map");
            return 0;
        }
        
        int num_recognized = 0;
        NCDValRef value;
        
        if (!NCDVal_IsInvalid(value = NCDVal_MapGetValue(options, "read_size"))) {
            uintmax_t read_size;
            if (!ncd_read_uintmax(value, &read_size) || read_size > SIZE_MAX || read_size == 0) {
                ModuleLog(i, BLOG_ERROR, "wrong read_size");
                return 0;
            }
            num_recognized++;
            *out_read_size = read_size;
        }
        
        if (NCDVal_MapCount(options) > num_recognized) {
            ModuleLog(i, BLOG_ERROR, "unrecognized options present");
            return 0;
        }
    }
    
    return 1;
}

static void connection_log (struct connection *o, int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    
    switch (o->type) {
        case CONNECTION_TYPE_CONNECT: {
            NCDModuleInst_Backend_LogVarArg(o->connect.i, BLOG_CURRENT_CHANNEL, level, fmt, vl);
        } break;
        
        case CONNECTION_TYPE_LISTEN: {
            if (BLog_WouldLog(BLOG_CURRENT_CHANNEL, level)) {
                BLog_Begin();
                o->listen.listen_inst->i->params->logfunc(o->listen.listen_inst->i);
                char addr_str[BADDR_MAX_PRINT_LEN];
                BAddr_Print(&o->listen.addr, addr_str);
                BLog_Append("client %s: ", addr_str);
                BLog_AppendVarArg(fmt, vl);
                BLog_Finish(BLOG_CURRENT_CHANNEL, level);
            }
        } break;
        
        default: ASSERT(0);
    }
    
    va_end(vl);
}

static void connection_free_connection (struct connection *o)
{
    // disconnect read instance
    if (o->read_inst) {
        ASSERT(o->read_inst->con_inst == o)
        o->read_inst->con_inst = NULL;
    }
    
    // disconnect write instance
    if (o->write_inst) {
        ASSERT(o->write_inst->con_inst == o)
        o->write_inst->con_inst = NULL;
    }
    
    // free connection interfaces
    BConnection_RecvAsync_Free(&o->connection);
    BConnection_SendAsync_Free(&o->connection);
    
    // free connection
    BConnection_Free(&o->connection);
    
    // free store
    NCDBufStore_Free(&o->store);
}

static void connection_error (struct connection *o)
{
    ASSERT(o->state == CONNECTION_STATE_CONNECTING ||
           o->state == CONNECTION_STATE_ESTABLISHED)
    
    // for listen clients, we don't report errors directly,
    // we just terminate the client process
    if (o->type == CONNECTION_TYPE_LISTEN) {
        ASSERT(o->state != CONNECTION_STATE_CONNECTING)
        connection_abort(o);
        return;
    }
    
    // free connector
    if (o->state == CONNECTION_STATE_CONNECTING) {
        BConnector_Free(&o->connect.connector);
    }
    
    // free connection resources
    if (o->state == CONNECTION_STATE_ESTABLISHED) {
        connection_free_connection(o);
    }
    
    // trigger reporting of failure
    if (o->state == CONNECTION_STATE_ESTABLISHED) {
        NCDModuleInst_Backend_Down(o->connect.i);
    }
    NCDModuleInst_Backend_Up(o->connect.i);
    
    // set state
    o->state = CONNECTION_STATE_ERROR;
}

static void connection_abort (struct connection *o)
{
    ASSERT(o->state == CONNECTION_STATE_ESTABLISHED)
    
    // free connection resources
    connection_free_connection(o);
    
    // if this is a listen connection, terminate client process
    if (o->type == CONNECTION_TYPE_LISTEN) {
        NCDModuleProcess_Terminate(&o->listen.process);
    }
    
    // set state
    o->state = CONNECTION_STATE_ABORTED;
}

static void connection_connector_handler (void *user, int is_error)
{
    struct connection *o = user;
    ASSERT(o->type == CONNECTION_TYPE_CONNECT)
    ASSERT(o->state == CONNECTION_STATE_CONNECTING)
    
    // check error
    if (is_error) {
        connection_log(o, BLOG_ERROR, "connection failed");
        goto fail;
    }
    
    // init connection
    if (!BConnection_Init(&o->connection, BConnection_source_connector(&o->connect.connector), o->connect.i->params->iparams->reactor, o, connection_connection_handler)) {
        connection_log(o, BLOG_ERROR, "BConnection_Init failed");
        goto fail;
    }
    
    // free connector
    BConnector_Free(&o->connect.connector);
    
    return connection_complete_establish(o);
    
fail:
    connection_error(o);
}

static void connection_complete_establish (struct connection *o)
{
    // init connection interfaces
    BConnection_SendAsync_Init(&o->connection);
    BConnection_RecvAsync_Init(&o->connection);
    
    // setup send/recv done callbacks
    StreamPassInterface_Sender_Init(BConnection_SendAsync_GetIf(&o->connection), connection_send_handler_done, o);
    StreamRecvInterface_Receiver_Init(BConnection_RecvAsync_GetIf(&o->connection), connection_recv_handler_done, o);
    
    // init store
    NCDBufStore_Init(&o->store, o->connect.read_buf_size);
    
    // set not reading, not writing, recv not closed
    o->read_inst = NULL;
    o->write_inst = NULL;
    o->recv_closed = 0;
    
    // set state
    o->state = CONNECTION_STATE_ESTABLISHED;
    
    // go up
    NCDModuleInst_Backend_Up(o->connect.i);
}

static void connection_connection_handler (void *user, int event)
{
    struct connection *o = user;
    ASSERT(o->state == CONNECTION_STATE_ESTABLISHED)
    ASSERT(event == BCONNECTION_EVENT_RECVCLOSED || event == BCONNECTION_EVENT_ERROR)
    ASSERT(event != BCONNECTION_EVENT_RECVCLOSED || !o->recv_closed)
    
    if (event == BCONNECTION_EVENT_RECVCLOSED) {
        // if we have read operation, make it finish with eof
        if (o->read_inst) {
            ASSERT(o->read_inst->con_inst == o)
            o->read_inst->con_inst = NULL;
            o->read_inst->read_size = 0;
            NCDModuleInst_Backend_Up(o->read_inst->i);
            o->read_inst = NULL;
        }
        
        // set recv closed
        o->recv_closed = 1;
        return;
    }
    
    connection_log(o, BLOG_ERROR, "connection error");
    
    // handle error
    connection_error(o);
}

static void connection_send_handler_done (void *user, int data_len)
{
    struct connection *o = user;
    ASSERT(o->state == CONNECTION_STATE_ESTABLISHED)
    ASSERT(o->write_inst)
    ASSERT(o->write_inst->con_inst == o)
    ASSERT(data_len > 0)
    ASSERT(data_len <= o->write_inst->data.len)
    
    struct write_instance *wr = o->write_inst;
    
    // update send state
    wr->data = MemRef_SubFrom(wr->data, data_len);
    
    // if there's more to send, send again
    if (wr->data.len > 0) {
        size_t to_send = (wr->data.len > INT_MAX) ? INT_MAX : wr->data.len;
        StreamPassInterface_Sender_Send(BConnection_SendAsync_GetIf(&o->connection), (uint8_t *)wr->data.ptr, to_send);
        return;
    }
    
    // finish write operation
    wr->con_inst = NULL;
    NCDModuleInst_Backend_Up(wr->i);
    o->write_inst = NULL;
}

static void connection_recv_handler_done (void *user, int data_len)
{
    struct connection *o = user;
    ASSERT(o->state == CONNECTION_STATE_ESTABLISHED)
    ASSERT(o->read_inst)
    ASSERT(o->read_inst->con_inst == o)
    ASSERT(!o->recv_closed)
    ASSERT(data_len > 0)
    ASSERT(data_len <= NCDBufStore_BufSize(&o->store))
    
    struct read_instance *re = o->read_inst;
    
    // finish read operation
    re->con_inst = NULL;
    re->read_size = data_len;
    NCDModuleInst_Backend_Up(re->i);
    o->read_inst = NULL;
}

static void connection_process_handler (struct NCDModuleProcess_s *process, int event)
{
    struct connection *o = UPPER_OBJECT(process, struct connection, listen.process);
    ASSERT(o->type == CONNECTION_TYPE_LISTEN)
    
    switch (event) {
        case NCDMODULEPROCESS_EVENT_UP: {
            ASSERT(o->state == CONNECTION_STATE_ESTABLISHED)
        } break;
        
        case NCDMODULEPROCESS_EVENT_DOWN: {
            ASSERT(o->state == CONNECTION_STATE_ESTABLISHED)
            NCDModuleProcess_Continue(&o->listen.process);
        } break;
        
        case NCDMODULEPROCESS_EVENT_TERMINATED: {
            ASSERT(o->state == CONNECTION_STATE_ABORTED)
            
            struct listen_instance *li = o->listen.listen_inst;
            ASSERT(!li->have_error)
            
            // remove from clients list
            LinkedList0_Remove(&li->clients_list, &o->listen.clients_list_node);
            
            // free process
            NCDModuleProcess_Free(&o->listen.process);
            
            // free connection structure
            free(o);
            
            // if listener is dying and this was the last process, have it die
            if (li->dying && LinkedList0_IsEmpty(&li->clients_list)) {
                NCDModuleInst_Backend_Dead(li->i);
            }
        } break;
        
        default: ASSERT(0);
    }
}

static int connection_process_func_getspecialobj (struct NCDModuleProcess_s *process, NCD_string_id_t name, NCDObject *out_object)
{
    struct connection *o = UPPER_OBJECT(process, struct connection, listen.process);
    ASSERT(o->type == CONNECTION_TYPE_LISTEN)
    
    if (name == ModuleString(o->listen.listen_inst->i, STRING_SOCKET)) {
        *out_object = NCDObject_Build(ModuleString(o->listen.listen_inst->i, STRING_SYS_SOCKET), o, connection_process_socket_obj_func_getvar, NCDObject_no_getobj);
        return 1;
    }
    
    if (name == NCD_STRING_CALLER) {
        *out_object = NCDObject_Build(-1, o, NCDObject_no_getvar, connection_process_caller_obj_func_getobj);
        return 1;
    }
    
    return 0;
}

static int connection_process_socket_obj_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value)
{
    struct connection *o = NCDObject_DataPtr(obj);
    ASSERT(o->type == CONNECTION_TYPE_LISTEN)
    
    if (name == ModuleString(o->listen.listen_inst->i, STRING_CLIENT_ADDR)) {
        *out_value = ncd_make_baddr(o->listen.addr, mem);
        if (NCDVal_IsInvalid(*out_value)) {
            connection_log(o, BLOG_ERROR, "ncd_make_baddr failed");
        }
        return 1;
    }
    
    return 0;
}

static int connection_process_caller_obj_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object)
{
    struct connection *o = NCDObject_DataPtr(obj);
    ASSERT(o->type == CONNECTION_TYPE_LISTEN)
    
    return NCDModuleInst_Backend_GetObj(o->listen.listen_inst->i, name, out_object);
}

static void listen_listener_handler (void *user)
{
    struct listen_instance *o = user;
    ASSERT(!o->have_error)
    ASSERT(!o->dying)
    
    // allocate connection structure
    struct connection *con = malloc(sizeof(*con));
    if (!con) {
        ModuleLog(o->i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // set connection type and listen instance
    con->type = CONNECTION_TYPE_LISTEN;
    con->listen.listen_inst = o;
    
    // init connection
    if (!BConnection_Init(&con->connection, BConnection_source_listener(&o->listener, &con->listen.addr), o->i->params->iparams->reactor, con, connection_connection_handler)) {
        ModuleLog(o->i, BLOG_ERROR, "BConnection_Init failed");
        goto fail1;
    }
    
    // init connection interfaces
    BConnection_SendAsync_Init(&con->connection);
    BConnection_RecvAsync_Init(&con->connection);
    
    // setup send/recv done callbacks
    StreamPassInterface_Sender_Init(BConnection_SendAsync_GetIf(&con->connection), connection_send_handler_done, con);
    StreamRecvInterface_Receiver_Init(BConnection_RecvAsync_GetIf(&con->connection), connection_recv_handler_done, con);
    
    // init process
    if (!NCDModuleProcess_InitValue(&con->listen.process, o->i, o->client_template, o->client_template_args, connection_process_handler)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDModuleProcess_InitValue failed");
        goto fail2;
    }
    
    // set special objects callback
    NCDModuleProcess_SetSpecialFuncs(&con->listen.process, connection_process_func_getspecialobj);
    
    // insert to clients list
    LinkedList0_Prepend(&o->clients_list, &con->listen.clients_list_node);
    
    // init store
    NCDBufStore_Init(&con->store, o->read_buf_size);
    
    // set not reading, not writing, recv not closed
    con->read_inst = NULL;
    con->write_inst = NULL;
    con->recv_closed = 0;
    
    // set state
    con->state = CONNECTION_STATE_ESTABLISHED;
    return;
    
fail2:
    BConnection_RecvAsync_Free(&con->connection);
    BConnection_SendAsync_Free(&con->connection);
    BConnection_Free(&con->connection);
fail1:
    free(con);
fail0:
    return;
}

static int connect_custom_addr_handler (void *user, NCDValRef protocol, NCDValRef data)
{
    NCDValRef *device_path = user;
    if (NCDVal_StringEquals(protocol, "device")) {
        if (!NCDVal_IsStringNoNulls(data)) {
            return 0;
        }
        *device_path = data;
        return 1;
    }
    return 0;
}

static void connect_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct connection *o = vo;
    o->type = CONNECTION_TYPE_CONNECT;
    o->connect.i = i;
    
    // pass connection pointer to methods so the same methods can work for
    // listen type connections
    NCDModuleInst_Backend_PassMemToMethods(i);
    
    // read arguments
    NCDValRef address_arg;
    NCDValRef options_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 1, &address_arg) &&
        !NCDVal_ListRead(params->args, 2, &address_arg, &options_arg)
    ) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // parse options
    if (!parse_options(i, options_arg, &o->connect.read_buf_size)) {
        goto fail0;
    }
    
    // read address
    struct BConnection_addr address;
    NCDValRef device_path;
    if (!ncd_read_bconnection_addr_ext(address_arg, connect_custom_addr_handler, &device_path, &address)) {
        ModuleLog(i, BLOG_ERROR, "wrong address");
        goto error;
    }
    
    // Did the custom handler handle the address as a device?
    if (address.type == -1) {
        // get null terminated device path
        NCDValNullTermString device_path_nts;
        if (!NCDVal_StringNullTerminate(device_path, &device_path_nts)) {
            ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
            goto error;
        }
        
        // open the device
        int devfd = open(device_path_nts.data, O_RDWR|O_NONBLOCK);
        NCDValNullTermString_Free(&device_path_nts);
        if (devfd < 0) {
            ModuleLog(i, BLOG_ERROR, "open failed");
            goto error;
        }
        
        // init connection
        if (!BConnection_Init(&o->connection, BConnection_source_pipe(devfd, 1), i->params->iparams->reactor, o, connection_connection_handler)) {
            ModuleLog(i, BLOG_ERROR, "BConnection_Init failed");
            goto error;
        }
        
        connection_complete_establish(o);
    } else {
        // init connector
        if (!BConnector_InitGeneric(&o->connect.connector, address, i->params->iparams->reactor, o, connection_connector_handler)) {
            ModuleLog(i, BLOG_ERROR, "BConnector_InitGeneric failed");
            goto error;
        }
        
        // set state
        o->state = CONNECTION_STATE_CONNECTING;
    }
    
    return;
    
error:
    // go up in error state
    o->state = CONNECTION_STATE_ERROR;
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void connect_func_die (void *vo)
{
    struct connection *o = vo;
    ASSERT(o->type == CONNECTION_TYPE_CONNECT)
    
    // free connector
    if (o->state == CONNECTION_STATE_CONNECTING) {
        BConnector_Free(&o->connect.connector);
    }
    
    // free connection resources
    if (o->state == CONNECTION_STATE_ESTABLISHED) {
        connection_free_connection(o);
    }
    
    NCDModuleInst_Backend_Dead(o->connect.i);
}

static int connect_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct connection *o = vo;
    ASSERT(o->type == CONNECTION_TYPE_CONNECT)
    ASSERT(o->state != CONNECTION_STATE_CONNECTING)
    
    if (name == NCD_STRING_IS_ERROR) {
        int is_error = (o->state == CONNECTION_STATE_ERROR);
        *out = ncd_make_boolean(mem, is_error);
        return 1;
    }
    
    return 0;
}

static void read_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct read_instance *o = vo;
    o->i = i;
    
    // read arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get connection
    struct connection *con_inst = params->method_user;
    
    // check connection state
    if (con_inst->state != CONNECTION_STATE_ESTABLISHED) {
        ModuleLog(i, BLOG_ERROR, "connection is not established");
        goto fail0;
    }
    
    // check if there's already a read in progress
    if (con_inst->read_inst) {
        ModuleLog(i, BLOG_ERROR, "read is already in progress");
        goto fail0;
    }
    
    // get buffer
    o->buf = NCDBufStore_GetBuf(&con_inst->store);
    if (!o->buf) {
        ModuleLog(i, BLOG_ERROR, "NCDBufStore_GetBuf failed");
        goto fail0;
    }
    
    // if eof was reached, go up immediately
    if (con_inst->recv_closed) {
        o->con_inst = NULL;
        o->read_size = 0;
        NCDModuleInst_Backend_Up(i);
        return;
    }
    
    // set connection
    o->con_inst = con_inst;
    
    // register read operation in connection
    con_inst->read_inst = o;
    
    // receive
    size_t buf_size = NCDBufStore_BufSize(&con_inst->store);
    int to_read = (buf_size > INT_MAX ? INT_MAX : buf_size);
    StreamRecvInterface_Receiver_Recv(BConnection_RecvAsync_GetIf(&con_inst->connection), (uint8_t *)NCDBuf_Data(o->buf), to_read);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void read_func_die (void *vo)
{
    struct read_instance *o = vo;
    
    // if we're receiving, abort connection
    if (o->con_inst) {
        ASSERT(o->con_inst->state == CONNECTION_STATE_ESTABLISHED)
        ASSERT(o->con_inst->read_inst == o)
        connection_abort(o->con_inst);
    }
    
    // release buffer
    BRefTarget_Deref(NCDBuf_RefTarget(o->buf));
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int read_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct read_instance *o = vo;
    ASSERT(!o->con_inst)
    
    if (name == NCD_STRING_EMPTY) {
        *out = NCDVal_NewExternalString(mem, NCDBuf_Data(o->buf), o->read_size, NCDBuf_RefTarget(o->buf));
        return 1;
    }
    
    if (name == NCD_STRING_EOF || name == NCD_STRING_NOT_EOF) {
        *out = ncd_make_boolean(mem, (o->read_size == 0) == (name == NCD_STRING_EOF));
        return 1;
    }
    
    return 0;
}

static void write_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct write_instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef data_arg;
    if (!NCDVal_ListRead(params->args, 1, &data_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(data_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // get connection
    struct connection *con_inst = params->method_user;
    
    // check connection state
    if (con_inst->state != CONNECTION_STATE_ESTABLISHED) {
        ModuleLog(i, BLOG_ERROR, "connection is not established");
        goto fail0;
    }
    
    // check if there's already a write in progress
    if (con_inst->write_inst) {
        ModuleLog(i, BLOG_ERROR, "write is already in progress");
        goto fail0;
    }
    
    // set send state
    o->data = NCDVal_StringMemRef(data_arg);
    
    // if there's nothing to send, go up immediately
    if (o->data.len == 0) {
        o->con_inst = NULL;
        NCDModuleInst_Backend_Up(i);
        return;
    }
    
    // set connection
    o->con_inst = con_inst;
    
    // register write operation in connection
    con_inst->write_inst = o;
    
    // send
    size_t to_send = (o->data.len > INT_MAX) ? INT_MAX : o->data.len;
    StreamPassInterface_Sender_Send(BConnection_SendAsync_GetIf(&con_inst->connection), (uint8_t *)o->data.ptr, to_send);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void write_func_die (void *vo)
{
    struct write_instance *o = vo;
    
    // if we're sending, abort connection
    if (o->con_inst) {
        ASSERT(o->con_inst->state == CONNECTION_STATE_ESTABLISHED)
        ASSERT(o->con_inst->write_inst == o)
        connection_abort(o->con_inst);
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void close_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // read arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get connection
    struct connection *con_inst = params->method_user;
    
    // check connection state
    if (con_inst->state != CONNECTION_STATE_ESTABLISHED) {
        ModuleLog(i, BLOG_ERROR, "connection is not established");
        goto fail0;
    }
    
    // go up
    NCDModuleInst_Backend_Up(i);
    
    // abort
    connection_abort(con_inst);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void listen_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct listen_instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef address_arg;
    NCDValRef client_template_arg;
    NCDValRef args_arg;
    NCDValRef options_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 3, &address_arg, &client_template_arg, &args_arg) &&
        !NCDVal_ListRead(params->args, 4, &address_arg, &client_template_arg, &args_arg, &options_arg)
    ) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(client_template_arg) || !NCDVal_IsList(args_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // parse options
    if (!parse_options(i, options_arg, &o->read_buf_size)) {
        goto fail0;
    }
    
    // remember client template and arguments
    o->client_template = client_template_arg;
    o->client_template_args = args_arg;
    
    // set no error, not dying
    o->have_error = 0;
    o->dying = 0;
    
    // read address
    struct BConnection_addr address;
    if (!ncd_read_bconnection_addr(address_arg, &address)) {
        ModuleLog(i, BLOG_ERROR, "wrong address");
        goto error;
    }
    
    // init listener
    if (!BListener_InitGeneric(&o->listener, address, i->params->iparams->reactor, o, listen_listener_handler)) {
        ModuleLog(i, BLOG_ERROR, "BListener_InitGeneric failed");
        goto error;
    }
    
    // init clients list
    LinkedList0_Init(&o->clients_list);
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
error:
    // go up with error
    o->have_error = 1;
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void listen_func_die (void *vo)
{
    struct listen_instance *o = vo;
    ASSERT(!o->dying)
    
    // free listener
    if (!o->have_error) {
        BListener_Free(&o->listener);
    }
    
    // if we have no clients, die right away
    if (o->have_error || LinkedList0_IsEmpty(&o->clients_list)) {
        NCDModuleInst_Backend_Dead(o->i);
        return;
    }
    
    // set dying
    o->dying = 1;
    
    // abort all clients and wait for them
    for (LinkedList0Node *ln = LinkedList0_GetFirst(&o->clients_list); ln; ln = LinkedList0Node_Next(ln)) {
        struct connection *con = UPPER_OBJECT(ln, struct connection, listen.clients_list_node);
        ASSERT(con->type == CONNECTION_TYPE_LISTEN)
        ASSERT(con->listen.listen_inst == o)
        ASSERT(con->state == CONNECTION_STATE_ESTABLISHED || con->state == CONNECTION_STATE_ABORTED)
        
        if (con->state != CONNECTION_STATE_ABORTED) {
            connection_abort(con);
        }
    }
}

static int listen_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct listen_instance *o = vo;
    
    if (name == NCD_STRING_IS_ERROR) {
        *out = ncd_make_boolean(mem, o->have_error);
        return 1;
    }
    
    return 0;
}

static struct NCDModule modules[] = {
    {
        .type = "sys.connect",
        .base_type = "sys.socket",
        .func_new2 = connect_func_new,
        .func_die = connect_func_die,
        .func_getvar2 = connect_func_getvar,
        .alloc_size = sizeof(struct connection)
    }, {
        .type = "sys.socket::read",
        .func_new2 = read_func_new,
        .func_die = read_func_die,
        .func_getvar2 = read_func_getvar,
        .alloc_size = sizeof(struct read_instance)
    }, {
        .type = "sys.socket::write",
        .func_new2 = write_func_new,
        .func_die = write_func_die,
        .alloc_size = sizeof(struct write_instance)
    }, {
        .type = "sys.socket::close",
        .func_new2 = close_func_new
    }, {
        .type = "sys.listen",
        .func_new2 = listen_func_new,
        .func_die = listen_func_die,
        .func_getvar2 = listen_func_getvar,
        .alloc_size = sizeof(struct listen_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_socket = {
    .modules = modules,
    .strings = strings
};
