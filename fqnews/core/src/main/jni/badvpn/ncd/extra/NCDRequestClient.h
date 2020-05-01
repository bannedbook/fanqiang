/**
 * @file NCDRequestClient.h
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

#ifndef BADVPN_NCDREQUESTCLIENT_H
#define BADVPN_NCDREQUESTCLIENT_H

#include <stdint.h>

#include <misc/debug.h>
#include <misc/debugerror.h>
#include <misc/debugcounter.h>
#include <structure/BAVL.h>
#include <base/DebugObject.h>
#include <system/BConnection.h>
#include <system/BConnectionGeneric.h>
#include <flow/PacketProtoDecoder.h>
#include <flow/PacketStreamSender.h>
#include <flow/PacketPassFifoQueue.h>
#include <ncd/NCDValGenerator.h>
#include <ncd/NCDValParser.h>

struct NCDRequestClient_req;

typedef void (*NCDRequestClient_handler_error) (void *user);
typedef void (*NCDRequestClient_handler_connected) (void *user);
typedef void (*NCDRequestClientRequest_handler_sent) (void *user);
typedef void (*NCDRequestClientRequest_handler_reply) (void *user, NCDValMem reply_mem, NCDValRef reply_value);
typedef void (*NCDRequestClientRequest_handler_finished) (void *user, int is_error);

typedef struct {
    BReactor *reactor;
    NCDStringIndex *string_index;
    void *user;
    NCDRequestClient_handler_error handler_error;
    NCDRequestClient_handler_connected handler_connected;
    BConnector connector;
    BConnection con;
    PacketPassFifoQueue send_queue;
    PacketStreamSender send_sender;
    PacketProtoDecoder recv_decoder;
    PacketPassInterface recv_if;
    BAVL reqs_tree;
    uint32_t next_request_id;
    int state;
    int is_error;
    DebugCounter d_reqests_ctr;
    DebugError d_err;
    DebugObject d_obj;
} NCDRequestClient;

typedef struct {
    NCDRequestClient *client;
    void *user;
    NCDRequestClientRequest_handler_sent handler_sent;
    NCDRequestClientRequest_handler_reply handler_reply;
    NCDRequestClientRequest_handler_finished handler_finished;
    struct NCDRequestClient_req *req;
    DebugError d_err;
    DebugObject d_obj;
} NCDRequestClientRequest;

struct NCDRequestClient_req {
    NCDRequestClientRequest *creq;
    NCDRequestClient *client;
    BAVLNode reqs_tree_node;
    uint32_t request_id;
    uint8_t *request_data;
    int request_len;
    PacketPassInterface *send_qflow_iface;
    PacketPassFifoQueueFlow send_qflow;
    int state;
};

int NCDRequestClient_Init (NCDRequestClient *o, struct BConnection_addr addr, BReactor *reactor, NCDStringIndex *string_index,
                           void *user,
                           NCDRequestClient_handler_error handler_error,
                           NCDRequestClient_handler_connected handler_connected) WARN_UNUSED;
void NCDRequestClient_Free (NCDRequestClient *o);

int NCDRequestClientRequest_Init (NCDRequestClientRequest *o, NCDRequestClient *client, NCDValRef payload_value, void *user,
                                  NCDRequestClientRequest_handler_sent handler_sent,
                                  NCDRequestClientRequest_handler_reply handler_reply,
                                  NCDRequestClientRequest_handler_finished handler_finished) WARN_UNUSED;
void NCDRequestClientRequest_Free (NCDRequestClientRequest *o);
void NCDRequestClientRequest_Abort (NCDRequestClientRequest *o);

#endif
