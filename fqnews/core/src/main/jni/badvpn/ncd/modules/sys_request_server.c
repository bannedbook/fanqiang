/**
 * @file sys_request_server.c
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
 * A simple a IPC interface for NCD to talk to other processes over a Unix socket.
 * 
 * Synopsis:
 *   sys.request_server(listen_address, string request_handler_template, list args)
 * 
 * Description:
 *   Initializes a request server on the given socket path. Requests are served by
 *   starting a template process for every request. Multiple such processes may
 *   exist simultaneously. Termination of these processess may be initiated at
 *   any time if the request server no longer needs the request in question served.
 *   The payload of a request is a value, and can be accessed as _request.data
 *   from within the handler process. Replies to the request can be sent using
 *   _request->reply(data); replies are values too. Finally, _request->finish()
 *   should be called to indicate that no further replies will be sent. Calling
 *   finish() will immediately initiate termination of the handler process.
 *   Requests can be sent to NCD using the badvpn-ncd-request program.
 * 
 *   The listen address should be in the same format as for the socket module.
 *   In particular, it must be in one of the following forms:
 *   - {"tcp", {"ipv4", ipv4_address, port_number}},
 *   - {"tcp", {"ipv6", ipv6_address, port_number}},
 *   - {"unix", socket_path}.
 * 
 * Predefined objects and variables in request_handler_template:
 *   _caller - provides access to objects as seen from the sys.request_server()
 *     command
 *   _request.data - the request payload as sent by the client
 *   string _request.client_addr - the address of the client. The form is
 *     like the second part of the sys.request_server() address format, e.g.
 *     {"ipv4", "1.2.3.4", "4000"}.
 * 
 * Synopsis:
 *   sys.request_server.request::reply(reply_data)
 * 
 * Synopsis:
 *   sys.request_server.request::finish()
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>
#include <errno.h>

#include <misc/offset.h>
#include <misc/debug.h>
#include <misc/byteorder.h>
#include <protocol/packetproto.h>
#include <protocol/requestproto.h>
#include <structure/LinkedList0.h>
#include <system/BConnection.h>
#include <system/BConnectionGeneric.h>
#include <system/BAddr.h>
#include <flow/PacketProtoDecoder.h>
#include <flow/PacketStreamSender.h>
#include <flow/PacketPassFifoQueue.h>
#include <ncd/NCDValParser.h>
#include <ncd/NCDValGenerator.h>
#include <ncd/extra/address_utils.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_sys_request_server.h>

#define SEND_PAYLOAD_MTU 32768
#define RECV_PAYLOAD_MTU 32768

#define SEND_MTU (SEND_PAYLOAD_MTU + sizeof(struct requestproto_header))
#define RECV_MTU (RECV_PAYLOAD_MTU + sizeof(struct requestproto_header))

struct instance {
    NCDModuleInst *i;
    NCDValRef request_handler_template;
    NCDValRef args;
    BListener listener;
    LinkedList0 connections_list;
    int dying;
};

#define CONNECTION_STATE_RUNNING 1
#define CONNECTION_STATE_TERMINATING 2

struct reply;

struct connection {
    struct instance *inst;
    LinkedList0Node connections_list_node;
    BConnection con;
    BAddr addr;
    PacketProtoDecoder recv_decoder;
    PacketPassInterface recv_if;
    PacketPassFifoQueue send_queue;
    PacketStreamSender send_pss;
    LinkedList0 requests_list;
    LinkedList0 replies_list;
    int state;
};

struct request {
    struct connection *con;
    uint32_t request_id;
    LinkedList0Node requests_list_node;
    NCDValMem request_data_mem;
    NCDValRef request_data;
    struct reply *end_reply;
    NCDModuleProcess process;
    int terminating;
    int got_finished;
};

struct reply {
    struct connection *con;
    LinkedList0Node replies_list_node;
    PacketPassFifoQueueFlow send_qflow;
    PacketPassInterface *send_qflow_if;
    uint8_t *send_buf;
};

static void listener_handler (struct instance *o);
static void connection_free (struct connection *c);
static void connection_free_link (struct connection *c);
static void connection_terminate (struct connection *c);
static void connection_con_handler (struct connection *c, int event);
static void connection_recv_decoder_handler_error (struct connection *c);
static void connection_recv_if_handler_send (struct connection *c, uint8_t *data, int data_len);
static int request_init (struct connection *c, uint32_t request_id, const uint8_t *data, int data_len);
static void request_free (struct request *r);
static struct request * find_request (struct connection *c, uint32_t request_id);
static void request_process_handler_event (NCDModuleProcess *process, int event);
static int request_process_func_getspecialobj (NCDModuleProcess *process, NCD_string_id_t name, NCDObject *out_object);
static int request_process_caller_obj_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object);
static int request_process_request_obj_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out_value);
static void request_terminate (struct request *r);
static struct reply * reply_init (struct connection *c, uint32_t request_id, NCDValRef reply_data);
static void reply_start (struct reply *r, uint32_t type);
static void reply_free (struct reply *r);
static void reply_send_qflow_if_handler_done (struct reply *r);
static void instance_free (struct instance *o);

enum {STRING_REQUEST, STRING_DATA, STRING_CLIENT_ADDR,
      STRING_SYS_REQUEST_SERVER_REQUEST};

static const char *strings[] = {
    "_request", "data", "client_addr",
    "sys.request_server.request", NULL
};

static void listener_handler (struct instance *o)
{
    ASSERT(!o->dying)
    
    BReactor *reactor = o->i->params->iparams->reactor;
    BPendingGroup *pg = BReactor_PendingGroup(reactor);
    
    struct connection *c = malloc(sizeof(*c));
    if (!c) {
        ModuleLog(o->i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    c->inst = o;
    
    LinkedList0_Prepend(&o->connections_list, &c->connections_list_node);
    
    if (!BConnection_Init(&c->con, BConnection_source_listener(&o->listener, &c->addr), reactor, c, (BConnection_handler)connection_con_handler)) {
        ModuleLog(o->i, BLOG_ERROR, "BConnection_Init failed");
        goto fail1;
    }
    
    BConnection_SendAsync_Init(&c->con);
    BConnection_RecvAsync_Init(&c->con);
    StreamPassInterface *con_send_if = BConnection_SendAsync_GetIf(&c->con);
    StreamRecvInterface *con_recv_if = BConnection_RecvAsync_GetIf(&c->con);
    
    PacketPassInterface_Init(&c->recv_if, RECV_MTU, (PacketPassInterface_handler_send)connection_recv_if_handler_send, c, pg);
    
    if (!PacketProtoDecoder_Init(&c->recv_decoder, con_recv_if, &c->recv_if, pg, c, (PacketProtoDecoder_handler_error)connection_recv_decoder_handler_error)) {
        ModuleLog(o->i, BLOG_ERROR, "PacketProtoDecoder_Init failed");
        goto fail2;
    }
    
    PacketStreamSender_Init(&c->send_pss, con_send_if, PACKETPROTO_ENCLEN(SEND_MTU), pg);
    
    PacketPassFifoQueue_Init(&c->send_queue, PacketStreamSender_GetInput(&c->send_pss), pg);
    
    LinkedList0_Init(&c->requests_list);
    
    LinkedList0_Init(&c->replies_list);
    
    c->state = CONNECTION_STATE_RUNNING;
    
    ModuleLog(o->i, BLOG_INFO, "connection initialized");
    return;
    
fail2:
    PacketPassInterface_Free(&c->recv_if);
    BConnection_RecvAsync_Free(&c->con);
    BConnection_SendAsync_Free(&c->con);
    BConnection_Free(&c->con);
fail1:
    LinkedList0_Remove(&o->connections_list, &c->connections_list_node);
    free(c);
fail0:
    return;
}

static void connection_free (struct connection *c)
{
    struct instance *o = c->inst;
    ASSERT(c->state == CONNECTION_STATE_TERMINATING)
    ASSERT(LinkedList0_IsEmpty(&c->requests_list))
    ASSERT(LinkedList0_IsEmpty(&c->replies_list))
    
    LinkedList0_Remove(&o->connections_list, &c->connections_list_node);
    free(c);
}

static void connection_free_link (struct connection *c)
{
    PacketPassFifoQueue_PrepareFree(&c->send_queue);
    
    LinkedList0Node *ln;
    while (ln = LinkedList0_GetFirst(&c->replies_list)) {
        struct reply *r = UPPER_OBJECT(ln, struct reply, replies_list_node);
        ASSERT(r->con == c)
        reply_free(r);
    }
    
    PacketPassFifoQueue_Free(&c->send_queue);
    PacketStreamSender_Free(&c->send_pss);
    PacketProtoDecoder_Free(&c->recv_decoder);
    PacketPassInterface_Free(&c->recv_if);
    BConnection_RecvAsync_Free(&c->con);
    BConnection_SendAsync_Free(&c->con);
    BConnection_Free(&c->con);
}

static void connection_terminate (struct connection *c)
{
    ASSERT(c->state == CONNECTION_STATE_RUNNING)
    
    for (LinkedList0Node *ln = LinkedList0_GetFirst(&c->requests_list); ln; ln = LinkedList0Node_Next(ln)) {
        struct request *r = UPPER_OBJECT(ln, struct request, requests_list_node);
        
        if (!r->terminating) {
            request_terminate(r);
        }
    }
    
    connection_free_link(c);
    
    c->state = CONNECTION_STATE_TERMINATING;
    
    if (LinkedList0_IsEmpty(&c->requests_list)) {
        connection_free(c);
        return;
    }
}

static void connection_con_handler (struct connection *c, int event)
{
    struct instance *o = c->inst;
    ASSERT(c->state == CONNECTION_STATE_RUNNING)
    
    ModuleLog(o->i, BLOG_INFO, "connection closed");
    
    connection_terminate(c);
}

static void connection_recv_decoder_handler_error (struct connection *c)
{
    struct instance *o = c->inst;
    ASSERT(c->state == CONNECTION_STATE_RUNNING)
    
    ModuleLog(o->i, BLOG_ERROR, "decoder error");
    
    connection_terminate(c);
}

static void connection_recv_if_handler_send (struct connection *c, uint8_t *data, int data_len)
{
    struct instance *o = c->inst;
    ASSERT(c->state == CONNECTION_STATE_RUNNING)
    ASSERT(data_len >= 0)
    ASSERT(data_len <= RECV_MTU)
    
    PacketPassInterface_Done(&c->recv_if);
    
    if (data_len < sizeof(struct requestproto_header)) {
        ModuleLog(o->i, BLOG_ERROR, "missing requestproto header");
        goto fail;
    }
    
    struct requestproto_header header;
    memcpy(&header, data, sizeof(header));
    uint32_t request_id = ltoh32(header.request_id);
    uint32_t type = ltoh32(header.type);
    
    switch (type) {
        case REQUESTPROTO_TYPE_CLIENT_REQUEST: {
            if (find_request(c, request_id)) {
                ModuleLog(o->i, BLOG_ERROR, "request with the same ID already exists");
                goto fail;
            }
            
            if (!request_init(c, request_id, data + sizeof(header), data_len - sizeof(header))) {
                goto fail;
            }
        } break;
        
        case REQUESTPROTO_TYPE_CLIENT_ABORT: {
            struct request *r = find_request(c, request_id);
            if (!r) {
                // this is expected if we finish before we get the abort
                return;
            }
            
            if (!r->terminating) {
                request_terminate(r);
            }
        } break;
        
        default:
            ModuleLog(o->i, BLOG_ERROR, "invalid requestproto type");
            goto fail;
    }
    
    return;
    
fail:
    connection_terminate(c);
}

static int request_init (struct connection *c, uint32_t request_id, const uint8_t *data, int data_len)
{
    struct instance *o = c->inst;
    ASSERT(c->state == CONNECTION_STATE_RUNNING)
    ASSERT(!find_request(c, request_id))
    ASSERT(data_len >= 0)
    ASSERT(data_len <= RECV_PAYLOAD_MTU)
    
    struct request *r = malloc(sizeof(*r));
    if (!r) {
        ModuleLog(o->i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    r->con = c;
    r->request_id = request_id;
    
    LinkedList0_Prepend(&c->requests_list, &r->requests_list_node);
    
    NCDValMem_Init(&r->request_data_mem, o->i->params->iparams->string_index);
    
    if (!NCDValParser_Parse(MemRef_Make((const char *)data, data_len), &r->request_data_mem, &r->request_data)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDValParser_Parse failed");
        goto fail1;
    }
    
    if (!(r->end_reply = reply_init(c, request_id, NCDVal_NewInvalid()))) {
        goto fail1;
    }
    
    if (!NCDModuleProcess_InitValue(&r->process, o->i, o->request_handler_template, o->args, request_process_handler_event)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDModuleProcess_Init failed");
        goto fail3;
    }
    
    NCDModuleProcess_SetSpecialFuncs(&r->process, request_process_func_getspecialobj);
    
    r->terminating = 0;
    r->got_finished = 0;
    
    ModuleLog(o->i, BLOG_INFO, "request initialized");
    return 1;
    
fail3:
    reply_free(r->end_reply);
fail1:
    NCDValMem_Free(&r->request_data_mem);
    LinkedList0_Remove(&c->requests_list, &r->requests_list_node);
    free(r);
fail0:
    return 0;
}

static void request_free (struct request *r)
{
    struct connection *c = r->con;
    NCDModuleProcess_AssertFree(&r->process);
    
    if (c->state != CONNECTION_STATE_TERMINATING) {
        uint32_t type = r->got_finished ? REQUESTPROTO_TYPE_SERVER_FINISHED : REQUESTPROTO_TYPE_SERVER_ERROR;
        reply_start(r->end_reply, type);
    }
    
    NCDModuleProcess_Free(&r->process);
    NCDValMem_Free(&r->request_data_mem);
    LinkedList0_Remove(&c->requests_list, &r->requests_list_node);
    free(r);
}

static struct request * find_request (struct connection *c, uint32_t request_id)
{
    for (LinkedList0Node *ln = LinkedList0_GetFirst(&c->requests_list); ln; ln = LinkedList0Node_Next(ln)) {
        struct request *r = UPPER_OBJECT(ln, struct request, requests_list_node);
        if (!r->terminating && r->request_id == request_id) {
            return r;
        }
    }
    
    return NULL;
}

static void request_process_handler_event (NCDModuleProcess *process, int event)
{
    struct request *r = UPPER_OBJECT(process, struct request, process);
    struct connection *c = r->con;
    struct instance *o = c->inst;
    
    switch (event) {
        case NCDMODULEPROCESS_EVENT_UP: {
            ASSERT(!r->terminating)
        } break;
        
        case NCDMODULEPROCESS_EVENT_DOWN: {
            ASSERT(!r->terminating)
            
            NCDModuleProcess_Continue(&r->process);
        } break;
        
        case NCDMODULEPROCESS_EVENT_TERMINATED: {
            ASSERT(r->terminating)
            
            request_free(r);
            
            if (c->state == CONNECTION_STATE_TERMINATING && LinkedList0_IsEmpty(&c->requests_list)) {
                connection_free(c);
                
                if (o->dying && LinkedList0_IsEmpty(&o->connections_list)) {
                    instance_free(o);
                    return;
                }
            }
        } break;
        
        default: ASSERT(0);
    }
}

static int request_process_func_getspecialobj (NCDModuleProcess *process, NCD_string_id_t name, NCDObject *out_object)
{
    struct request *r = UPPER_OBJECT(process, struct request, process);
    
    if (name == NCD_STRING_CALLER) {
        *out_object = NCDObject_Build(-1, r, NCDObject_no_getvar, request_process_caller_obj_func_getobj);
        return 1;
    }
    
    if (name == ModuleString(r->con->inst->i, STRING_REQUEST)) {
        *out_object = NCDObject_Build(ModuleString(r->con->inst->i, STRING_SYS_REQUEST_SERVER_REQUEST), r, request_process_request_obj_func_getvar, NCDObject_no_getobj);
        return 1;
    }
    
    return 0;
}

static int request_process_caller_obj_func_getobj (const NCDObject *obj, NCD_string_id_t name, NCDObject *out_object)
{
    struct request *r = NCDObject_DataPtr(obj);
    
    return NCDModuleInst_Backend_GetObj(r->con->inst->i, name, out_object);
}

static int request_process_request_obj_func_getvar (const NCDObject *obj, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct request *r = NCDObject_DataPtr(obj);
    
    if (name == ModuleString(r->con->inst->i, STRING_DATA)) {
        *out = NCDVal_NewCopy(mem, r->request_data);
        return 1;
    }
    
    if (name == ModuleString(r->con->inst->i, STRING_CLIENT_ADDR)) {
        *out = ncd_make_baddr(r->con->addr, mem);
        return 1;
    }
    
    return 0;
}

static void request_terminate (struct request *r)
{
    ASSERT(!r->terminating)
    
    NCDModuleProcess_Terminate(&r->process);
    
    r->terminating = 1;
}

static struct reply * reply_init (struct connection *c, uint32_t request_id, NCDValRef reply_data)
{
    struct instance *o = c->inst;
    ASSERT(c->state == CONNECTION_STATE_RUNNING)
    NCDVal_Assert(reply_data);
    
    struct reply *r = malloc(sizeof(*r));
    if (!r) {
        ModuleLog(o->i, BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    r->con = c;
    
    LinkedList0_Prepend(&c->replies_list, &r->replies_list_node);
    
    PacketPassFifoQueueFlow_Init(&r->send_qflow, &c->send_queue);
    
    r->send_qflow_if = PacketPassFifoQueueFlow_GetInput(&r->send_qflow);
    PacketPassInterface_Sender_Init(r->send_qflow_if, (PacketPassInterface_handler_done)reply_send_qflow_if_handler_done, r);
    
    ExpString str;
    if (!ExpString_Init(&str)) {
        ModuleLog(o->i, BLOG_ERROR, "ExpString_Init failed");
        goto fail1;
    }
    
    if (!ExpString_AppendZeros(&str, sizeof(struct packetproto_header) + sizeof(struct requestproto_header))) {
        ModuleLog(o->i, BLOG_ERROR, "ExpString_AppendZeros failed");
        goto fail2;
    }
    
    if (!NCDVal_IsInvalid(reply_data) && !NCDValGenerator_AppendGenerate(reply_data, &str)) {
        ModuleLog(o->i, BLOG_ERROR, "NCDValGenerator_AppendGenerate failed");
        goto fail2;
    }
    
    size_t len = ExpString_Length(&str);
    if (len > INT_MAX || len > PACKETPROTO_ENCLEN(SEND_MTU) || len - sizeof(struct packetproto_header) > UINT16_MAX) {
        ModuleLog(o->i, BLOG_ERROR, "reply is too long");
        goto fail2;
    }
    
    r->send_buf = (uint8_t *)ExpString_Get(&str);
    
    struct packetproto_header pp;
    pp.len = htol16(len - sizeof(pp));
    
    struct requestproto_header rp;
    rp.request_id = htol32(request_id);
    
    memcpy(r->send_buf, &pp, sizeof(pp));
    memcpy(r->send_buf + sizeof(pp), &rp, sizeof(rp));
    
    return r;
    
fail2:
    ExpString_Free(&str);
fail1:
    PacketPassFifoQueueFlow_Free(&r->send_qflow);
    LinkedList0_Remove(&c->replies_list, &r->replies_list_node);
    free(r);
fail0:
    return NULL;
}

static void reply_start (struct reply *r, uint32_t type)
{
    struct requestproto_header rp;
    memcpy(&rp, r->send_buf + sizeof(struct packetproto_header), sizeof(rp));
    rp.type = htol32(type);
    memcpy(r->send_buf + sizeof(struct packetproto_header), &rp, sizeof(rp));
    
    struct packetproto_header pp;
    memcpy(&pp, r->send_buf, sizeof(pp));
    
    int len = ltoh16(pp.len) + sizeof(struct packetproto_header);
    
    PacketPassInterface_Sender_Send(r->send_qflow_if, r->send_buf, len);
}

static void reply_free (struct reply *r)
{
    struct connection *c = r->con;
    PacketPassFifoQueueFlow_AssertFree(&r->send_qflow);
    
    free(r->send_buf);
    PacketPassFifoQueueFlow_Free(&r->send_qflow);
    LinkedList0_Remove(&c->replies_list, &r->replies_list_node);
    free(r);
}

static void reply_send_qflow_if_handler_done (struct reply *r)
{
    reply_free(r);
}

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef listen_addr_arg;
    NCDValRef request_handler_template_arg;
    NCDValRef args_arg;
    if (!NCDVal_ListRead(params->args, 3, &listen_addr_arg, &request_handler_template_arg, &args_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(request_handler_template_arg) || !NCDVal_IsList(args_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    o->request_handler_template = request_handler_template_arg;
    o->args = args_arg;
    
    // read listen address
    struct BConnection_addr addr;
    if (!ncd_read_bconnection_addr(listen_addr_arg, &addr)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong listen address");
        goto fail0;
    }
    
    // init listener
    if (!BListener_InitGeneric(&o->listener, addr, i->params->iparams->reactor, o, (BListener_handler)listener_handler)) {
        ModuleLog(o->i, BLOG_ERROR, "BListener_InitGeneric failed");
        goto fail0;
    }
    
    // init connections list
    LinkedList0_Init(&o->connections_list);
    
    // set not dying
    o->dying = 0;
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void instance_free (struct instance *o)
{
    ASSERT(o->dying)
    ASSERT(LinkedList0_IsEmpty(&o->connections_list))
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void func_die (void *vo)
{
    struct instance *o = vo;
    ASSERT(!o->dying)
    
    // free listener
    BListener_Free(&o->listener);
    
    // terminate connections
    LinkedList0Node *next_ln;
    for (LinkedList0Node *ln = LinkedList0_GetFirst(&o->connections_list); ln && (next_ln = LinkedList0Node_Next(ln)), ln; ln = next_ln) { 
        struct connection *c = UPPER_OBJECT(ln, struct connection, connections_list_node);
        ASSERT(c->inst == o)
        
        if (c->state != CONNECTION_STATE_TERMINATING) {
            connection_terminate(c);
        }
    }
    
    // set dying
    o->dying = 1;
    
    // if no connections, die right away
    if (LinkedList0_IsEmpty(&o->connections_list)) {
        instance_free(o);
        return;
    }
}

static void reply_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    NCDValRef reply_data;
    if (!NCDVal_ListRead(params->args, 1, &reply_data)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail;
    }
    
    NCDModuleInst_Backend_Up(i);
    
    struct request *r = params->method_user;
    struct connection *c = r->con;
    
    if (r->terminating) {
        ModuleLog(i, BLOG_ERROR, "request is dying, cannot submit reply");
        goto fail;
    }
    
    struct reply *rpl = reply_init(c, r->request_id, reply_data);
    if (!rpl) {
        ModuleLog(i, BLOG_ERROR, "failed to submit reply");
        goto fail;
    }
    
    reply_start(rpl, REQUESTPROTO_TYPE_SERVER_REPLY);
    return;
    
fail:
    NCDModuleInst_Backend_DeadError(i);
}

static void finish_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail;
    }
    
    NCDModuleInst_Backend_Up(i);
    
    struct request *r = params->method_user;
    
    if (r->terminating) {
        ModuleLog(i, BLOG_ERROR, "request is dying, cannot submit finished");
        goto fail;
    }
    
    r->got_finished = 1;
    
    request_terminate(r);
    return;
    
fail:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "sys.request_server",
        .func_new2 = func_new,
        .func_die = func_die,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "sys.request_server.request::reply",
        .func_new2 = reply_func_new
    }, {
        .type = "sys.request_server.request::finish",
        .func_new2 = finish_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_sys_request_server = {
    .modules = modules,
    .strings = strings
};
