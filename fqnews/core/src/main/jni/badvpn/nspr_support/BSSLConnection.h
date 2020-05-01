/**
 * @file BSSLConnection.h
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

#ifndef BADVPN_BSSLCONNECTION_H
#define BADVPN_BSSLCONNECTION_H

#include <prio.h>
#include <nss/ssl.h>

#include <misc/debug.h>
#include <misc/debugerror.h>
#include <base/DebugObject.h>
#include <base/BPending.h>
#include <base/BMutex.h>
#include <flow/StreamPassInterface.h>
#include <flow/StreamRecvInterface.h>
#include <threadwork/BThreadWork.h>

#define BSSLCONNECTION_EVENT_UP 1
#define BSSLCONNECTION_EVENT_ERROR 2

#define BSSLCONNECTION_BUF_SIZE 4096

#define BSSLCONNECTION_FLAG_THREADWORK_HANDSHAKE (1 << 0)
#define BSSLCONNECTION_FLAG_THREADWORK_IO (1 << 1)

typedef void (*BSSLConnection_handler) (void *user, int event);

struct BSSLConnection_backend;

typedef struct {
    PRFileDesc *prfd;
    BPendingGroup *pg;
    void *user;
    BSSLConnection_handler handler;
    struct BSSLConnection_backend *backend;
    int have_error;
    int up;
    BPending init_job;
    StreamPassInterface send_if;
    StreamRecvInterface recv_if;
    BPending recv_job;
    const uint8_t *send_data;
    int send_len;
    uint8_t *recv_data;
    int recv_avail;
#ifndef NDEBUG
    int user_io_started;
    int releasebuffers_called;
#endif
    DebugError d_err;
    DebugObject d_obj;
} BSSLConnection;

struct BSSLConnection_backend {
    StreamPassInterface *send_if;
    StreamRecvInterface *recv_if;
    BThreadWorkDispatcher *twd;
    int flags;
    BSSLConnection *con;
    uint8_t send_buf[BSSLCONNECTION_BUF_SIZE];
    int send_busy;
    int send_pos;
    int send_len;
    uint8_t recv_buf[BSSLCONNECTION_BUF_SIZE];
    int recv_busy;
    int recv_pos;
    int recv_len;
    int threadwork_state;
    int threadwork_want_recv;
    int threadwork_want_send;
    BThreadWork threadwork;
    SECStatus threadwork_result_sec;
    PRInt32 threadwork_result_pr;
    PRErrorCode threadwork_error;
    BMutex send_buf_mutex;
    BMutex recv_buf_mutex;
};

int BSSLConnection_GlobalInit (void) WARN_UNUSED;
int BSSLConnection_MakeBackend (PRFileDesc *prfd, StreamPassInterface *send_if, StreamRecvInterface *recv_if, BThreadWorkDispatcher *twd, int flags) WARN_UNUSED;

void BSSLConnection_Init (BSSLConnection *o, PRFileDesc *prfd, int force_handshake, BPendingGroup *pg, void *user,
                          BSSLConnection_handler handler);
void BSSLConnection_Free (BSSLConnection *o);
void BSSLConnection_ReleaseBuffers (BSSLConnection *o);
StreamPassInterface * BSSLConnection_GetSendIf (BSSLConnection *o);
StreamRecvInterface * BSSLConnection_GetRecvIf (BSSLConnection *o);

#endif
