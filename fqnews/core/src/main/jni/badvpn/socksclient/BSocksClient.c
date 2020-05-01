/**
 * @file BSocksClient.c
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

#include <string.h>

#include <misc/byteorder.h>
#include <misc/balloc.h>
#include <base/BLog.h>

#include <socksclient/BSocksClient.h>

#include <generated/blog_channel_BSocksClient.h>

#define STATE_CONNECTING 1
#define STATE_SENDING_HELLO 2
#define STATE_SENT_HELLO 3
#define STATE_SENDING_PASSWORD 10
#define STATE_SENT_PASSWORD 11
#define STATE_SENDING_REQUEST 4
#define STATE_SENT_REQUEST 5
#define STATE_RECEIVED_REPLY_HEADER 6
#define STATE_UP 7

static void report_error (BSocksClient *o, int error);
static void init_control_io (BSocksClient *o);
static void free_control_io (BSocksClient *o);
static void init_up_io (BSocksClient *o);
static void free_up_io (BSocksClient *o);
static int reserve_buffer (BSocksClient *o, bsize_t size);
static void start_receive (BSocksClient *o, uint8_t *dest, int total);
static void do_receive (BSocksClient *o);
static void connector_handler (BSocksClient* o, int is_error);
static void connection_handler (BSocksClient* o, int event);
static void recv_handler_done (BSocksClient *o, int data_len);
static void send_handler_done (BSocksClient *o);
static void auth_finished (BSocksClient *p);

void report_error (BSocksClient *o, int error)
{
    DEBUGERROR(&o->d_err, o->handler(o->user, error))
}

void init_control_io (BSocksClient *o)
{
    // init receiving
    BConnection_RecvAsync_Init(&o->con);
    o->control.recv_if = BConnection_RecvAsync_GetIf(&o->con);
    StreamRecvInterface_Receiver_Init(o->control.recv_if, (StreamRecvInterface_handler_done)recv_handler_done, o);
    
    // init sending
    BConnection_SendAsync_Init(&o->con);
    PacketStreamSender_Init(&o->control.send_sender, BConnection_SendAsync_GetIf(&o->con), INT_MAX, BReactor_PendingGroup(o->reactor));
    o->control.send_if = PacketStreamSender_GetInput(&o->control.send_sender);
    PacketPassInterface_Sender_Init(o->control.send_if, (PacketPassInterface_handler_done)send_handler_done, o);
}

void free_control_io (BSocksClient *o)
{
    // free sending
    PacketStreamSender_Free(&o->control.send_sender);
    BConnection_SendAsync_Free(&o->con);
    
    // free receiving
    BConnection_RecvAsync_Free(&o->con);
}

void init_up_io (BSocksClient *o)
{
    // init receiving
    BConnection_RecvAsync_Init(&o->con);
    
    // init sending
    BConnection_SendAsync_Init(&o->con);
}

void free_up_io (BSocksClient *o)
{
    // free sending
    BConnection_SendAsync_Free(&o->con);
    
    // free receiving
    BConnection_RecvAsync_Free(&o->con);
}

int reserve_buffer (BSocksClient *o, bsize_t size)
{
    if (size.is_overflow) {
        BLog(BLOG_ERROR, "size overflow");
        return 0;
    }
    
    char *buffer = (char *)BRealloc(o->buffer, size.value);
    if (!buffer) {
        BLog(BLOG_ERROR, "BRealloc failed");
        return 0;
    }
    
    o->buffer = buffer;
    
    return 1;
}

void start_receive (BSocksClient *o, uint8_t *dest, int total)
{
    ASSERT(total > 0)
    
    o->control.recv_dest = dest;
    o->control.recv_len = 0;
    o->control.recv_total = total;
    
    do_receive(o);
}

void do_receive (BSocksClient *o)
{
    ASSERT(o->control.recv_len < o->control.recv_total)
    
    StreamRecvInterface_Receiver_Recv(o->control.recv_if, o->control.recv_dest + o->control.recv_len, o->control.recv_total - o->control.recv_len);
}

void connector_handler (BSocksClient* o, int is_error)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state == STATE_CONNECTING)
    
    // check connection result
    if (is_error) {
        BLog(BLOG_ERROR, "connection failed");
        goto fail0;
    }
    
    // init connection
    if (!BConnection_Init(&o->con, BConnection_source_connector(&o->connector), o->reactor, o, (BConnection_handler)connection_handler)) {
        BLog(BLOG_ERROR, "BConnection_Init failed");
        goto fail0;
    }
    
    BLog(BLOG_DEBUG, "connected");
    
    // init control I/O
    init_control_io(o);
    
    // check number of methods
    if (o->num_auth_info == 0 || o->num_auth_info > 255) {
        BLog(BLOG_ERROR, "invalid number of authentication methods");
        goto fail1;
    }
    
    // allocate buffer for sending hello
    bsize_t size = bsize_add(
        bsize_fromsize(sizeof(struct socks_client_hello_header)), 
        bsize_mul(
            bsize_fromsize(o->num_auth_info),
            bsize_fromsize(sizeof(struct socks_client_hello_method))
        )
    );
    if (!reserve_buffer(o, size)) {
        goto fail1;
    }
    
    // write hello header
    struct socks_client_hello_header header;
    header.ver = hton8(SOCKS_VERSION);
    header.nmethods = hton8(o->num_auth_info);
    memcpy(o->buffer, &header, sizeof(header));
    
    // write hello methods
    for (size_t i = 0; i < o->num_auth_info; i++) {
        struct socks_client_hello_method method;
        method.method = hton8(o->auth_info[i].auth_type);
        memcpy(o->buffer + sizeof(header) + i * sizeof(method), &method, sizeof(method));
    }
    
    // send
    PacketPassInterface_Sender_Send(o->control.send_if, (uint8_t *)o->buffer, size.value);
    
    // set state
    o->state = STATE_SENDING_HELLO;
    
    return;
    
fail1:
    free_control_io(o);
    BConnection_Free(&o->con);
fail0:
    report_error(o, BSOCKSCLIENT_EVENT_ERROR);
    return;
}

void connection_handler (BSocksClient* o, int event)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->state != STATE_CONNECTING)
    
    if (o->state == STATE_UP && event == BCONNECTION_EVENT_RECVCLOSED) {
        report_error(o, BSOCKSCLIENT_EVENT_ERROR_CLOSED);
        return;
    }
    
    report_error(o, BSOCKSCLIENT_EVENT_ERROR);
    return;
}

void recv_handler_done (BSocksClient *o, int data_len)
{
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->control.recv_total - o->control.recv_len)
    DebugObject_Access(&o->d_obj);
    
    o->control.recv_len += data_len;
    
    if (o->control.recv_len < o->control.recv_total) {
        do_receive(o);
        return;
    }
    
    switch (o->state) {
        case STATE_SENT_HELLO: {
            BLog(BLOG_DEBUG, "received hello");
            
            struct socks_server_hello imsg;
            memcpy(&imsg, o->buffer, sizeof(imsg));
            
            if (ntoh8(imsg.ver) != SOCKS_VERSION) {
                BLog(BLOG_NOTICE, "wrong version");
                goto fail;
            }
            
            size_t auth_index;
            for (auth_index = 0; auth_index < o->num_auth_info; auth_index++) {
                if (o->auth_info[auth_index].auth_type == ntoh8(imsg.method)) {
                    break;
                }
            }
            
            if (auth_index == o->num_auth_info) {
                BLog(BLOG_NOTICE, "server didn't accept any authentication method");
                goto fail;
            }
            
            const struct BSocksClient_auth_info *ai = &o->auth_info[auth_index];
            
            switch (ai->auth_type) {
                case SOCKS_METHOD_NO_AUTHENTICATION_REQUIRED: {
                    BLog(BLOG_DEBUG, "no authentication");
                    
                    auth_finished(o);
                } break;
                
                case SOCKS_METHOD_USERNAME_PASSWORD: {
                    BLog(BLOG_DEBUG, "password authentication");
                    
                    if (ai->password.username_len == 0 || ai->password.username_len > 255 ||
                        ai->password.password_len == 0 || ai->password.password_len > 255
                    ) {
                        BLog(BLOG_NOTICE, "invalid username/password length");
                        goto fail;
                    }
                    
                    // allocate password packet
                    bsize_t size = bsize_fromsize(1 + 1 + ai->password.username_len + 1 + ai->password.password_len);
                    if (!reserve_buffer(o, size)) {
                        goto fail;
                    }
                    
                    // write password packet
                    char *ptr = o->buffer;
                    *ptr++ = 1;
                    *ptr++ = ai->password.username_len;
                    memcpy(ptr, ai->password.username, ai->password.username_len);
                    ptr += ai->password.username_len;
                    *ptr++ = ai->password.password_len;
                    memcpy(ptr, ai->password.password, ai->password.password_len);
                    ptr += ai->password.password_len;
                    
                    // start sending
                    PacketPassInterface_Sender_Send(o->control.send_if, (uint8_t *)o->buffer, size.value);
                    
                    // set state
                    o->state = STATE_SENDING_PASSWORD;
                } break;
                
                default: ASSERT(0);
            }
        } break;
        
        case STATE_SENT_REQUEST: {
            BLog(BLOG_DEBUG, "received reply header");
            
            struct socks_reply_header imsg;
            memcpy(&imsg, o->buffer, sizeof(imsg));
            
            if (ntoh8(imsg.ver) != SOCKS_VERSION) {
                BLog(BLOG_NOTICE, "wrong version");
                goto fail;
            }
            
            if (ntoh8(imsg.rep) != SOCKS_REP_SUCCEEDED) {
                BLog(BLOG_NOTICE, "reply not successful");
                goto fail;
            }
            
            int addr_len;
            switch (ntoh8(imsg.atyp)) {
                case SOCKS_ATYP_IPV4:
                    addr_len = sizeof(struct socks_addr_ipv4);
                    break;
                case SOCKS_ATYP_IPV6:
                    addr_len = sizeof(struct socks_addr_ipv6);
                    break;
                default:
                    BLog(BLOG_NOTICE, "reply has unknown address type");
                    goto fail;
            }
            
            // receive the rest of the reply
            start_receive(o, (uint8_t *)o->buffer + sizeof(imsg), addr_len);
            
            // set state
            o->state = STATE_RECEIVED_REPLY_HEADER;
        } break;
        
        case STATE_SENT_PASSWORD: {
            BLog(BLOG_DEBUG, "received password reply");
            
            if (o->buffer[0] != 1) {
                BLog(BLOG_NOTICE, "password reply has unknown version");
                goto fail;
            }
            
            if (o->buffer[1] != 0) {
                BLog(BLOG_NOTICE, "password reply is negative");
                goto fail;
            }
            
            auth_finished(o);
        } break;
        
        case STATE_RECEIVED_REPLY_HEADER: {
            BLog(BLOG_DEBUG, "received reply rest");
            
            // free buffer
            BFree(o->buffer);
            o->buffer = NULL;
            
            // free control I/O
            free_control_io(o);
            
            // init up I/O
            init_up_io(o);
            
            // set state
            o->state = STATE_UP;
            
            // call handler
            o->handler(o->user, BSOCKSCLIENT_EVENT_UP);
            return;
        } break;
        
        default:
            ASSERT(0);
    }
    
    return;
    
fail:
    report_error(o, BSOCKSCLIENT_EVENT_ERROR);
}

void send_handler_done (BSocksClient *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->buffer)
    
    switch (o->state) {
        case STATE_SENDING_HELLO: {
            BLog(BLOG_DEBUG, "sent hello");
            
            // allocate buffer for receiving hello
            bsize_t size = bsize_fromsize(sizeof(struct socks_server_hello));
            if (!reserve_buffer(o, size)) {
                goto fail;
            }
            
            // receive hello
            start_receive(o, (uint8_t *)o->buffer, size.value);
            
            // set state
            o->state = STATE_SENT_HELLO;
        } break;
        
        case STATE_SENDING_REQUEST: {
            BLog(BLOG_DEBUG, "sent request");
            
            // allocate buffer for receiving reply
            bsize_t size = bsize_add(
                bsize_fromsize(sizeof(struct socks_reply_header)),
                bsize_max(bsize_fromsize(sizeof(struct socks_addr_ipv4)), bsize_fromsize(sizeof(struct socks_addr_ipv6)))
            );
            if (!reserve_buffer(o, size)) {
                goto fail;
            }
            
            // receive reply header
            start_receive(o, (uint8_t *)o->buffer, sizeof(struct socks_reply_header));
            
            // set state
            o->state = STATE_SENT_REQUEST;
        } break;
        
        case STATE_SENDING_PASSWORD: {
            BLog(BLOG_DEBUG, "send password");
            
            // allocate buffer for receiving reply
            bsize_t size = bsize_fromsize(2);
            if (!reserve_buffer(o, size)) {
                goto fail;
            }
            
            // receive reply header
            start_receive(o, (uint8_t *)o->buffer, size.value);
            
            // set state
            o->state = STATE_SENT_PASSWORD;
        } break;
        
        default:
            ASSERT(0);
    }
    
    return;
    
fail:
    report_error(o, BSOCKSCLIENT_EVENT_ERROR);
}

void auth_finished (BSocksClient *o)
{
    // allocate request buffer
    bsize_t size = bsize_fromsize(sizeof(struct socks_request_header));
    switch (o->dest_addr.type) {
        case BADDR_TYPE_IPV4: size = bsize_add(size, bsize_fromsize(sizeof(struct socks_addr_ipv4))); break;
        case BADDR_TYPE_IPV6: size = bsize_add(size, bsize_fromsize(sizeof(struct socks_addr_ipv6))); break;
    }
    if (!reserve_buffer(o, size)) {
        report_error(o, BSOCKSCLIENT_EVENT_ERROR);
        return;
    }
    
    // write request
    struct socks_request_header header;
    header.ver = hton8(SOCKS_VERSION);
    header.cmd = hton8(SOCKS_CMD_CONNECT);
    header.rsv = hton8(0);
    switch (o->dest_addr.type) {
        case BADDR_TYPE_IPV4: {
            header.atyp = hton8(SOCKS_ATYP_IPV4);
            struct socks_addr_ipv4 addr;
            addr.addr = o->dest_addr.ipv4.ip;
            addr.port = o->dest_addr.ipv4.port;
            memcpy(o->buffer + sizeof(header), &addr, sizeof(addr));
        } break;
        case BADDR_TYPE_IPV6: {
            header.atyp = hton8(SOCKS_ATYP_IPV6);
            struct socks_addr_ipv6 addr;
            memcpy(addr.addr, o->dest_addr.ipv6.ip, sizeof(o->dest_addr.ipv6.ip));
            addr.port = o->dest_addr.ipv6.port;
            memcpy(o->buffer + sizeof(header), &addr, sizeof(addr));
        } break;
        default:
            ASSERT(0);
    }
    memcpy(o->buffer, &header, sizeof(header));
    
    // send request
    PacketPassInterface_Sender_Send(o->control.send_if, (uint8_t *)o->buffer, size.value);
    
    // set state
    o->state = STATE_SENDING_REQUEST;
}

struct BSocksClient_auth_info BSocksClient_auth_none (void)
{
    struct BSocksClient_auth_info info;
    info.auth_type = SOCKS_METHOD_NO_AUTHENTICATION_REQUIRED;
    return info;
}

struct BSocksClient_auth_info BSocksClient_auth_password (const char *username, size_t username_len, const char *password, size_t password_len)
{
    struct BSocksClient_auth_info info;
    info.auth_type = SOCKS_METHOD_USERNAME_PASSWORD;
    info.password.username = username;
    info.password.username_len = username_len;
    info.password.password = password;
    info.password.password_len = password_len;
    return info;
}

int BSocksClient_Init (BSocksClient *o,
                       BAddr server_addr, const struct BSocksClient_auth_info *auth_info, size_t num_auth_info,
                       BAddr dest_addr, BSocksClient_handler handler, void *user, BReactor *reactor)
{
    ASSERT(!BAddr_IsInvalid(&server_addr))
    ASSERT(dest_addr.type == BADDR_TYPE_IPV4 || dest_addr.type == BADDR_TYPE_IPV6)
#ifndef NDEBUG
    for (size_t i = 0; i < num_auth_info; i++) {
        ASSERT(auth_info[i].auth_type == SOCKS_METHOD_NO_AUTHENTICATION_REQUIRED ||
               auth_info[i].auth_type == SOCKS_METHOD_USERNAME_PASSWORD)
    }
#endif
    
    // init arguments
    o->auth_info = auth_info;
    o->num_auth_info = num_auth_info;
    o->dest_addr = dest_addr;
    o->handler = handler;
    o->user = user;
    o->reactor = reactor;
    
    // set no buffer
    o->buffer = NULL;
    
    // init connector
    if (!BConnector_Init(&o->connector, server_addr, o->reactor, o, (BConnector_handler)connector_handler)) {
        BLog(BLOG_ERROR, "BConnector_Init failed");
        goto fail0;
    }
    
    // set state
    o->state = STATE_CONNECTING;
    
    DebugError_Init(&o->d_err, BReactor_PendingGroup(o->reactor));
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail0:
    return 0;
}

void BSocksClient_Free (BSocksClient *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
    
    if (o->state != STATE_CONNECTING) {
        if (o->state == STATE_UP) {
            // free up I/O
            free_up_io(o);
        } else {
            // free control I/O
            free_control_io(o);
        }
        
        // free connection
        BConnection_Free(&o->con);
    }
    
    // free connector
    BConnector_Free(&o->connector);
    
    // free buffer
    if (o->buffer) {
        BFree(o->buffer);
    }
}

StreamPassInterface * BSocksClient_GetSendInterface (BSocksClient *o)
{
    ASSERT(o->state == STATE_UP)
    DebugObject_Access(&o->d_obj);
    
    return BConnection_SendAsync_GetIf(&o->con);
}

StreamRecvInterface * BSocksClient_GetRecvInterface (BSocksClient *o)
{
    ASSERT(o->state == STATE_UP)
    DebugObject_Access(&o->d_obj);
    
    return BConnection_RecvAsync_GetIf(&o->con);
}
