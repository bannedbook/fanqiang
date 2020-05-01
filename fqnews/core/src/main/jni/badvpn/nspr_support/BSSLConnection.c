/**
 * @file BSSLConnection.c
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

#include <prerror.h>
#include <nss/ssl.h>

#include <string.h>
#include <stdlib.h>

#include <misc/print_macros.h>
#include <base/BLog.h>

#include "BSSLConnection.h"

#include <generated/blog_channel_BSSLConnection.h>

#define THREADWORK_STATE_NONE 0
#define THREADWORK_STATE_HANDSHAKE 1
#define THREADWORK_STATE_READ 2
#define THREADWORK_STATE_WRITE 3

static void backend_threadwork_start (struct BSSLConnection_backend *b, int op);
static int backend_threadwork_do_io (struct BSSLConnection_backend *b);
static void connection_init_job_handler (BSSLConnection *o);
static void connection_init_up (BSSLConnection *o);
static void connection_try_io (BSSLConnection *o);
static void connection_threadwork_func_work (void *user);
static void connection_threadwork_handler_done (void *user);
static void connection_recv_job_handler (BSSLConnection *o);
static void connection_try_handshake (BSSLConnection *o);
static void connection_try_send (BSSLConnection *o);
static void connection_try_recv (BSSLConnection *o);
static void connection_send_if_handler_send (BSSLConnection *o, uint8_t *data, int data_len);
static void connection_recv_if_handler_recv (BSSLConnection *o, uint8_t *data, int data_len);

int bprconnection_initialized = 0;
PRDescIdentity bprconnection_identity;

static PRFileDesc * get_bottom (PRFileDesc *layer)
{
    while (layer->lower) {
        layer = layer->lower;
    }
    
    return layer;
}

static PRStatus method_close (PRFileDesc *fd)
{
    struct BSSLConnection_backend *b = (struct BSSLConnection_backend *)fd->secret;
    ASSERT(!b->con)
    ASSERT(b->threadwork_state == THREADWORK_STATE_NONE)
    
    // free mutexes
    if ((b->flags & BSSLCONNECTION_FLAG_THREADWORK_HANDSHAKE) || (b->flags & BSSLCONNECTION_FLAG_THREADWORK_IO)) {
        BMutex_Free(&b->recv_buf_mutex);
        BMutex_Free(&b->send_buf_mutex);
    }
    
    // free backend
    free(b);
    
    // set no secret
    fd->secret = NULL;
    
    return PR_SUCCESS;
}

static PRInt32 method_read (PRFileDesc *fd, void *buf, PRInt32 amount)
{
    struct BSSLConnection_backend *b = (struct BSSLConnection_backend *)fd->secret;
    ASSERT(amount > 0)
    
    if (b->threadwork_state != THREADWORK_STATE_NONE) {
        BMutex_Lock(&b->recv_buf_mutex);
    }
    
    // if we are receiving into buffer or buffer has no data left, refuse recv
    if (b->recv_busy || b->recv_pos == b->recv_len) {
        if (b->threadwork_state != THREADWORK_STATE_NONE) {
            b->threadwork_want_recv = 1;
            BMutex_Unlock(&b->recv_buf_mutex);
        } else {
            // start receiving if not already
            if (!b->recv_busy) {
                // set recv busy
                b->recv_busy = 1;
                
                // receive into buffer
                StreamRecvInterface_Receiver_Recv(b->recv_if, b->recv_buf, BSSLCONNECTION_BUF_SIZE);
            }
        }
        PR_SetError(PR_WOULD_BLOCK_ERROR, 0);
        return -1;
    }
    
    // limit amount to available data
    if (amount > b->recv_len - b->recv_pos) {
        amount = b->recv_len - b->recv_pos;
    }
    
    // copy data
    memcpy(buf, b->recv_buf + b->recv_pos, amount);
    
    // update buffer
    b->recv_pos += amount;
    
    if (b->threadwork_state != THREADWORK_STATE_NONE) {
        BMutex_Unlock(&b->recv_buf_mutex);
    }
    
    return amount;
}

static PRInt32 method_write (PRFileDesc *fd, const void *buf, PRInt32 amount)
{
    struct BSSLConnection_backend *b = (struct BSSLConnection_backend *)fd->secret;
    ASSERT(amount > 0)
    
    if (b->threadwork_state != THREADWORK_STATE_NONE) {
        BMutex_Lock(&b->send_buf_mutex);
    }
    
    ASSERT(!b->send_busy || b->send_pos < b->send_len)
    
    // if there is data in buffer, refuse send
    if (b->send_pos < b->send_len) {
        if (b->threadwork_state != THREADWORK_STATE_NONE) {
            b->threadwork_want_send = 1;
            BMutex_Unlock(&b->send_buf_mutex);
        }
        PR_SetError(PR_WOULD_BLOCK_ERROR, 0);
        return -1;
    }
    
    // limit amount to buffer size
    if (amount > BSSLCONNECTION_BUF_SIZE) {
        amount = BSSLCONNECTION_BUF_SIZE;
    }
    
    // init buffer
    memcpy(b->send_buf, buf, amount);
    b->send_pos = 0;
    b->send_len = amount;
    
    if (b->threadwork_state != THREADWORK_STATE_NONE) {
        BMutex_Unlock(&b->send_buf_mutex);
    } else {
        // start sending
        b->send_busy = 1;
        StreamPassInterface_Sender_Send(b->send_if, b->send_buf + b->send_pos, b->send_len - b->send_pos);
    }
    
    return amount;
}

static PRStatus method_shutdown (PRFileDesc *fd, PRIntn how)
{
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return PR_FAILURE;
}

static PRInt32 method_recv (PRFileDesc *fd, void *buf, PRInt32 amount, PRIntn flags, PRIntervalTime timeout)
{
    ASSERT(flags == 0)
    
    return method_read(fd, buf, amount);
}

static PRInt32 method_send (PRFileDesc *fd, const void *buf, PRInt32 amount, PRIntn flags, PRIntervalTime timeout)
{
    ASSERT(flags == 0)
    
    return method_write(fd, buf, amount);
}

static PRInt16 method_poll (PRFileDesc *fd, PRInt16 in_flags, PRInt16 *out_flags)
{
    *out_flags = 0;
    return in_flags;
}

static PRStatus method_getpeername (PRFileDesc *fd, PRNetAddr *addr)
{
    memset(addr, 0, sizeof(*addr));
    addr->raw.family = PR_AF_INET;
    return PR_SUCCESS;
}

static PRStatus method_getsocketoption (PRFileDesc *fd, PRSocketOptionData *data)
{
    switch (data->option) {
        case PR_SockOpt_Nonblocking:
            data->value.non_blocking = PR_TRUE;
            return PR_SUCCESS;
    }
    
    PR_SetError(PR_UNKNOWN_ERROR, 0);
    return PR_FAILURE;
}

static PRStatus method_setsocketoption (PRFileDesc *fd, const PRSocketOptionData *data)
{
    PR_SetError(PR_UNKNOWN_ERROR, 0);
    return PR_FAILURE;
}

static PRIntn _PR_InvalidIntn (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRInt32 _PR_InvalidInt32 (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRInt64 _PR_InvalidInt64 (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PROffset32 _PR_InvalidOffset32 (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PROffset64 _PR_InvalidOffset64 (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRStatus _PR_InvalidStatus (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return PR_FAILURE;
}

static PRFileDesc *_PR_InvalidDesc (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return NULL;
}

static PRIOMethods methods = {
    (PRDescType)0,
    method_close,
    method_read,
    method_write,
    (PRAvailableFN)_PR_InvalidInt32,
    (PRAvailable64FN)_PR_InvalidInt64,
    (PRFsyncFN)_PR_InvalidStatus,
    (PRSeekFN)_PR_InvalidOffset32,
    (PRSeek64FN)_PR_InvalidOffset64,
    (PRFileInfoFN)_PR_InvalidStatus,
    (PRFileInfo64FN)_PR_InvalidStatus,
    (PRWritevFN)_PR_InvalidInt32,
    (PRConnectFN)_PR_InvalidStatus,
    (PRAcceptFN)_PR_InvalidDesc,
    (PRBindFN)_PR_InvalidStatus,
    (PRListenFN)_PR_InvalidStatus,
    method_shutdown,
    method_recv,
    method_send,
    (PRRecvfromFN)_PR_InvalidInt32,
    (PRSendtoFN)_PR_InvalidInt32,
    method_poll,
    (PRAcceptreadFN)_PR_InvalidInt32,
    (PRTransmitfileFN)_PR_InvalidInt32,
    (PRGetsocknameFN)_PR_InvalidStatus,
    method_getpeername,
    (PRReservedFN)_PR_InvalidIntn,
    (PRReservedFN)_PR_InvalidIntn,
    method_getsocketoption,
    method_setsocketoption,
    (PRSendfileFN)_PR_InvalidInt32,
    (PRConnectcontinueFN)_PR_InvalidStatus,
    (PRReservedFN)_PR_InvalidIntn,
    (PRReservedFN)_PR_InvalidIntn,
    (PRReservedFN)_PR_InvalidIntn,
    (PRReservedFN)_PR_InvalidIntn
};

static void backend_send_if_handler_done (struct BSSLConnection_backend *b, int data_len)
{
    ASSERT(b->send_busy)
    ASSERT(b->send_len > 0)
    ASSERT(b->send_pos < b->send_len)
    ASSERT(data_len > 0)
    ASSERT(data_len <= b->send_len - b->send_pos)
    
    if (b->threadwork_state != THREADWORK_STATE_NONE) {
        BMutex_Lock(&b->send_buf_mutex);
    }
    
    // update buffer
    b->send_pos += data_len;
    
    // send more if needed
    if (b->send_pos < b->send_len) {
        StreamPassInterface_Sender_Send(b->send_if, b->send_buf + b->send_pos, b->send_len - b->send_pos);
        if (b->threadwork_state != THREADWORK_STATE_NONE) {
            BMutex_Unlock(&b->send_buf_mutex);
        }
        return;
    }
    
    // set send not busy
    b->send_busy = 0;
    
    if (b->threadwork_state != THREADWORK_STATE_NONE) {
        BMutex_Unlock(&b->send_buf_mutex);
    }
    
    // notify connection
    if (b->con && !b->con->have_error) {
        connection_try_io(b->con);
        return;
    }
}

static void backend_recv_if_handler_done (struct BSSLConnection_backend *b, int data_len)
{
    ASSERT(b->recv_busy)
    ASSERT(data_len > 0)
    ASSERT(data_len <= BSSLCONNECTION_BUF_SIZE)
    
    if (b->threadwork_state != THREADWORK_STATE_NONE) {
        BMutex_Lock(&b->recv_buf_mutex);
    }
    
    // init buffer
    b->recv_busy = 0;
    b->recv_pos = 0;
    b->recv_len = data_len;
    
    if (b->threadwork_state != THREADWORK_STATE_NONE) {
        BMutex_Unlock(&b->recv_buf_mutex);
    }
    
    // notify connection
    if (b->con && !b->con->have_error) {
        connection_try_io(b->con);
        return;
    }
}

static void backend_threadwork_start (struct BSSLConnection_backend *b, int op)
{
    ASSERT(b->con)
    ASSERT(b->threadwork_state == THREADWORK_STATE_NONE)
    ASSERT(op == THREADWORK_STATE_HANDSHAKE || op == THREADWORK_STATE_READ || op == THREADWORK_STATE_WRITE)
    
    b->threadwork_state = op;
    b->threadwork_want_recv = 0;
    b->threadwork_want_send = 0;
    BThreadWork_Init(&b->threadwork, b->twd, connection_threadwork_handler_done, b->con, connection_threadwork_func_work, b->con);
}

static int backend_threadwork_do_io (struct BSSLConnection_backend *b)
{
    ASSERT(b->con)
    ASSERT(b->threadwork_state == THREADWORK_STATE_NONE)
    
    int io_ready = (b->threadwork_want_recv && !b->recv_busy && b->recv_pos < b->recv_len) ||
                   (b->threadwork_want_send && b->send_pos == b->send_len);
    
    if (b->threadwork_want_recv && b->recv_pos == b->recv_len && !b->recv_busy) {
        b->recv_busy = 1;
        StreamRecvInterface_Receiver_Recv(b->recv_if, b->recv_buf, BSSLCONNECTION_BUF_SIZE);
    }
    
    if (b->send_pos < b->send_len && !b->send_busy) {
        b->send_busy = 1;
        StreamPassInterface_Sender_Send(b->send_if, b->send_buf + b->send_pos, b->send_len - b->send_pos);
    }
    
    return io_ready;
}

static void connection_report_error (BSSLConnection *o)
{
    ASSERT(!o->have_error)
    
    // set error
    o->have_error = 1;
    
    // report error
    DEBUGERROR(&o->d_err, o->handler(o->user, BSSLCONNECTION_EVENT_ERROR));
}

static void connection_init_job_handler (BSSLConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->have_error)
    ASSERT(!o->up)
    
    connection_try_handshake(o);
}

static void connection_init_up (BSSLConnection *o)
{
    // unset init job
    // (just in the impossible case that handshake completed before the init job executed)
    BPending_Unset(&o->init_job);
    
    // init send interface
    StreamPassInterface_Init(&o->send_if, (StreamPassInterface_handler_send)connection_send_if_handler_send, o, o->pg);
    
    // init recv interface
    StreamRecvInterface_Init(&o->recv_if, (StreamRecvInterface_handler_recv)connection_recv_if_handler_recv, o, o->pg);
    
    // init recv job
    BPending_Init(&o->recv_job, o->pg, (BPending_handler)connection_recv_job_handler, o);
    
    // set no send data
    o->send_len = -1;
    
    // set no recv data
    o->recv_avail = -1;
    
    // set up
    o->up = 1;
}

static void connection_try_io (BSSLConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->have_error)
    
    if (!o->up) {
        connection_try_handshake(o);
        return;
    }
    
    if (o->send_len > 0) {
        if (o->recv_avail > 0) {
            BPending_Set(&o->recv_job);
        }
        
        connection_try_send(o);
        return;
    }
    
    if (o->recv_avail > 0) {
        connection_try_recv(o);
        return;
    }
}

static void connection_threadwork_func_work (void *user)
{
    BSSLConnection *o = (BSSLConnection *)user;
    struct BSSLConnection_backend *b = o->backend;
    ASSERT(b->threadwork_state != THREADWORK_STATE_NONE)
    
    switch (b->threadwork_state) {
        case THREADWORK_STATE_HANDSHAKE:
            b->threadwork_result_sec = SSL_ForceHandshake(o->prfd);
            break;
        case THREADWORK_STATE_WRITE:
            b->threadwork_result_pr = PR_Write(o->prfd, o->send_data, o->send_len);
            break;
        case THREADWORK_STATE_READ:
            b->threadwork_result_pr = PR_Read(o->prfd, o->recv_data, o->recv_avail);
            break;
        default:
            ASSERT(0);
    }
    
    b->threadwork_error = PR_GetError();
}

static void connection_threadwork_handler_done (void *user)
{
    BSSLConnection *o = (BSSLConnection *)user;
    struct BSSLConnection_backend *b = o->backend;
    ASSERT(b->threadwork_state != THREADWORK_STATE_NONE)
    
    // remember what operation the threadwork was performing
    int op = b->threadwork_state;
    
    // free threadwork
    BThreadWork_Free(&b->threadwork);
    b->threadwork_state = THREADWORK_STATE_NONE;
    
    // start any necessary backend I/O operations, and determine if any of the requested
    // backend I/O that was not available at the time is now available
    int io_ready = backend_threadwork_do_io(b);
    
    switch (op) {
        case THREADWORK_STATE_HANDSHAKE: {
            ASSERT(!o->up)
            ASSERT((b->flags & BSSLCONNECTION_FLAG_THREADWORK_HANDSHAKE))
            
            if (b->threadwork_result_sec == SECFailure) {
                if (b->threadwork_error == PR_WOULD_BLOCK_ERROR) {
                    if (io_ready) {
                        // requested backend I/O got ready, try again
                        backend_threadwork_start(o->backend, THREADWORK_STATE_HANDSHAKE);
                    }
                    return;
                }
                BLog(BLOG_ERROR, "SSL_ForceHandshake failed (%"PRIi32")", b->threadwork_error);
                connection_report_error(o);
                return;
            }
            
            // init up
            connection_init_up(o);
            
            // report up
            o->handler(o->user, BSSLCONNECTION_EVENT_UP);
            return;
        } break;
        
        case THREADWORK_STATE_WRITE: {
            ASSERT(o->up)
            ASSERT((b->flags & BSSLCONNECTION_FLAG_THREADWORK_IO))
            ASSERT(o->send_len > 0)
            
            PRInt32 result = b->threadwork_result_pr;
            PRErrorCode error = b->threadwork_error;
            
            if (result < 0) {
                if (error == PR_WOULD_BLOCK_ERROR) {
                    if (io_ready) {
                        // requested backend I/O got ready, try again
                        backend_threadwork_start(o->backend, THREADWORK_STATE_WRITE);
                    } else if (o->recv_avail > 0) {
                        // don't forget about receiving
                        backend_threadwork_start(o->backend, THREADWORK_STATE_READ);
                    }
                    return;
                }
                BLog(BLOG_ERROR, "PR_Write failed (%"PRIi32")", error);
                connection_report_error(o);
                return;
            }
            
            ASSERT(result > 0)
            ASSERT(result <= o->send_len)
            
            // set no send data
            o->send_len = -1;
            
            // don't forget about receiving
            if (o->recv_avail > 0) {
                backend_threadwork_start(o->backend, THREADWORK_STATE_READ);
            }
            
            // finish send operation
            StreamPassInterface_Done(&o->send_if, result);
        } break;
        
        case THREADWORK_STATE_READ: {
            ASSERT(o->up)
            ASSERT((b->flags & BSSLCONNECTION_FLAG_THREADWORK_IO))
            ASSERT(o->recv_avail > 0)
            
            PRInt32 result = b->threadwork_result_pr;
            PRErrorCode error = b->threadwork_error;
            
            if (result < 0) {
                if (error == PR_WOULD_BLOCK_ERROR) {
                    if (io_ready) {
                        // requested backend I/O got ready, try again
                        backend_threadwork_start(o->backend, THREADWORK_STATE_READ);
                    } else if (o->send_len > 0) {
                        // don't forget about sending
                        backend_threadwork_start(o->backend, THREADWORK_STATE_WRITE);
                    }
                    return;
                }
                BLog(BLOG_ERROR, "PR_Read failed (%"PRIi32")", error);
                connection_report_error(o);
                return;
            }
            
            if (result == 0) {
                BLog(BLOG_ERROR, "PR_Read returned 0");
                connection_report_error(o);
                return;
            }
            
            ASSERT(result > 0)
            ASSERT(result <= o->recv_avail)
            
            // set no recv data
            o->recv_avail = -1;
            
            // don't forget about sending
            if (o->send_len > 0) {
                backend_threadwork_start(o->backend, THREADWORK_STATE_WRITE);
            }
            
            // finish receive operation
            StreamRecvInterface_Done(&o->recv_if, result);
        } break;
        
        default:
            ASSERT(0);
    }
    
    return;
}

static void connection_recv_job_handler (BSSLConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->have_error)
    ASSERT(o->up)
    ASSERT(o->recv_avail > 0)
    
    connection_try_recv(o);
    return;
}

static void connection_try_handshake (BSSLConnection *o)
{
    ASSERT(!o->have_error)
    ASSERT(!o->up)
    
    // continue in threadwork if requested
    if ((o->backend->flags & BSSLCONNECTION_FLAG_THREADWORK_HANDSHAKE)) {
        if (o->backend->threadwork_state == THREADWORK_STATE_NONE) {
            backend_threadwork_start(o->backend, THREADWORK_STATE_HANDSHAKE);
        }
        return;
    }
    
    // try handshake
    SECStatus res = SSL_ForceHandshake(o->prfd);
    if (res == SECFailure) {
        PRErrorCode error = PR_GetError();
        if (error == PR_WOULD_BLOCK_ERROR) {
            return;
        }
        BLog(BLOG_ERROR, "SSL_ForceHandshake failed (%"PRIi32")", error);
        connection_report_error(o);
        return;
    }
    
    // init up
    connection_init_up(o);
    
    // report up
    o->handler(o->user, BSSLCONNECTION_EVENT_UP);
    return;
}

static void connection_try_send (BSSLConnection *o)
{
    ASSERT(!o->have_error)
    ASSERT(o->up)
    ASSERT(o->send_len > 0)
    
    // continue in threadwork if requested
    if ((o->backend->flags & BSSLCONNECTION_FLAG_THREADWORK_IO)) {
        if (o->backend->threadwork_state == THREADWORK_STATE_NONE) {
            backend_threadwork_start(o->backend, THREADWORK_STATE_WRITE);
        }
        return;
    }
    
    // send
    PRInt32 res = PR_Write(o->prfd, o->send_data, o->send_len);
    if (res < 0) {
        PRErrorCode error = PR_GetError();
        if (error == PR_WOULD_BLOCK_ERROR) {
            return;
        }
        BLog(BLOG_ERROR, "PR_Write failed (%"PRIi32")", error);
        connection_report_error(o);
        return;
    }
    
    ASSERT(res > 0)
    ASSERT(res <= o->send_len)
    
    // set no send data
    o->send_len = -1;
    
    // done
    StreamPassInterface_Done(&o->send_if, res);
}

static void connection_try_recv (BSSLConnection *o)
{
    ASSERT(!o->have_error)
    ASSERT(o->up)
    ASSERT(o->recv_avail > 0)
    
    // unset recv job
    BPending_Unset(&o->recv_job);
    
    // continue in threadwork if requested
    if ((o->backend->flags & BSSLCONNECTION_FLAG_THREADWORK_IO)) {
        if (o->backend->threadwork_state == THREADWORK_STATE_NONE) {
            backend_threadwork_start(o->backend, THREADWORK_STATE_READ);
        }
        return;
    }
    
    // recv
    PRInt32 res = PR_Read(o->prfd, o->recv_data, o->recv_avail);
    if (res < 0) {
        PRErrorCode error = PR_GetError();
        if (error == PR_WOULD_BLOCK_ERROR) {
            return;
        }
        BLog(BLOG_ERROR, "PR_Read failed (%"PRIi32")", error);
        connection_report_error(o);
        return;
    }
    
    if (res == 0) {
        BLog(BLOG_ERROR, "PR_Read returned 0");
        connection_report_error(o);
        return;
    }
    
    ASSERT(res > 0)
    ASSERT(res <= o->recv_avail)
    
    // set no recv data
    o->recv_avail = -1;
    
    // done
    StreamRecvInterface_Done(&o->recv_if, res);
}

static void connection_send_if_handler_send (BSSLConnection *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->have_error)
    ASSERT(o->up)
    ASSERT(o->send_len == -1)
    ASSERT(data_len > 0)
    
#ifndef NDEBUG
    ASSERT(!o->releasebuffers_called)
    o->user_io_started = 1;
#endif
    
    // limit amount for PR_Write
    if (data_len > INT32_MAX) {
        data_len = INT32_MAX;
    }
    
    // set send data
    o->send_data = data;
    o->send_len = data_len;
    
    // start sending
    connection_try_send(o);
}

static void connection_recv_if_handler_recv (BSSLConnection *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->have_error)
    ASSERT(o->up)
    ASSERT(o->recv_avail == -1)
    ASSERT(data_len > 0)
    
#ifndef NDEBUG
    ASSERT(!o->releasebuffers_called)
    o->user_io_started = 1;
#endif
    
    // limit amount for PR_Read
    if (data_len > INT32_MAX) {
        data_len = INT32_MAX;
    }
    
    // set recv data
    o->recv_data = data;
    o->recv_avail = data_len;
    
    // start receiving
    connection_try_recv(o);
}

int BSSLConnection_GlobalInit (void)
{
    ASSERT(!bprconnection_initialized)
    
    if ((bprconnection_identity = PR_GetUniqueIdentity("BSSLConnection")) == PR_INVALID_IO_LAYER) {
        BLog(BLOG_ERROR, "PR_GetUniqueIdentity failed");
        return 0;
    }
    
    bprconnection_initialized = 1;
    
    return 1;
}

int BSSLConnection_MakeBackend (PRFileDesc *prfd, StreamPassInterface *send_if, StreamRecvInterface *recv_if, BThreadWorkDispatcher *twd, int flags)
{
    ASSERT(bprconnection_initialized)
    ASSERT(!(flags & ~(BSSLCONNECTION_FLAG_THREADWORK_HANDSHAKE | BSSLCONNECTION_FLAG_THREADWORK_IO)))
    ASSERT(!(flags & BSSLCONNECTION_FLAG_THREADWORK_HANDSHAKE) || twd)
    ASSERT(!(flags & BSSLCONNECTION_FLAG_THREADWORK_IO) || twd)
    
    // don't do stuff in threads if threads aren't available
    if (((flags & BSSLCONNECTION_FLAG_THREADWORK_HANDSHAKE) || (flags & BSSLCONNECTION_FLAG_THREADWORK_IO)) &&
        !BThreadWorkDispatcher_UsingThreads(twd)
    ) {
        BLog(BLOG_WARNING, "SSL operations in threads requested but threads are not available");
        flags &= ~(BSSLCONNECTION_FLAG_THREADWORK_HANDSHAKE | BSSLCONNECTION_FLAG_THREADWORK_IO);
    }
    
    // allocate backend
    struct BSSLConnection_backend *b = (struct BSSLConnection_backend *)malloc(sizeof(*b));
    if (!b) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail0;
    }
    
    // init mutexes
    if ((flags & BSSLCONNECTION_FLAG_THREADWORK_HANDSHAKE) || (flags & BSSLCONNECTION_FLAG_THREADWORK_IO)) {
        if (!BMutex_Init(&b->send_buf_mutex)) {
            BLog(BLOG_ERROR, "BMutex_Init failed");
            goto fail1;
        }
        
        if (!BMutex_Init(&b->recv_buf_mutex)) {
            BLog(BLOG_ERROR, "BMutex_Init failed");
            goto fail2;
        }
    }
    
    // init arguments
    b->send_if = send_if;
    b->recv_if = recv_if;
    b->twd = twd;
    b->flags = flags;
    
    // init interfaces
    StreamPassInterface_Sender_Init(b->send_if, (StreamPassInterface_handler_done)backend_send_if_handler_done, b);
    StreamRecvInterface_Receiver_Init(b->recv_if, (StreamRecvInterface_handler_done)backend_recv_if_handler_done, b);
    
    // set no connection
    b->con = NULL;
    
    // init send buffer
    b->send_busy = 0;
    b->send_len = 0;
    b->send_pos = 0;
    
    // init recv buffer
    b->recv_busy = 0;
    b->recv_pos = 0;
    b->recv_len = 0;
    
    // set threadwork state
    b->threadwork_state = THREADWORK_STATE_NONE;
    
    // init prfd
    memset(prfd, 0, sizeof(*prfd));
    prfd->methods = &methods;
    prfd->secret = (PRFilePrivate *)b;
    prfd->identity = bprconnection_identity;
    
    return 1;
    
    if ((flags & BSSLCONNECTION_FLAG_THREADWORK_HANDSHAKE) || (flags & BSSLCONNECTION_FLAG_THREADWORK_IO)) {
fail2:
        BMutex_Free(&b->send_buf_mutex);
    }
fail1:
    free(b);
fail0:
    return 0;
}

void BSSLConnection_Init (BSSLConnection *o, PRFileDesc *prfd, int force_handshake, BPendingGroup *pg, void *user,
                          BSSLConnection_handler handler)
{
    ASSERT(force_handshake == 0 || force_handshake == 1)
    ASSERT(handler)
    ASSERT(bprconnection_initialized)
    ASSERT(get_bottom(prfd)->identity == bprconnection_identity)
    ASSERT(!((struct BSSLConnection_backend *)(get_bottom(prfd)->secret))->con)
    
    // init arguments
    o->prfd = prfd;
    o->pg = pg;
    o->user = user;
    o->handler = handler;
    
    // set backend
    o->backend = (struct BSSLConnection_backend *)(get_bottom(prfd)->secret);
    ASSERT(!o->backend->con)
    ASSERT(o->backend->threadwork_state == THREADWORK_STATE_NONE)
    
    // set have no error
    o->have_error = 0;
    
    // init init job
    BPending_Init(&o->init_job, o->pg, (BPending_handler)connection_init_job_handler, o);
    
    if (force_handshake) {
        // set not up
        o->up = 0;
        
        // set init job
        BPending_Set(&o->init_job);
    } else {
        // init up
        connection_init_up(o);
    }
    
    // set backend connection
    o->backend->con = o;
    
#ifndef NDEBUG
    o->user_io_started = 0;
    o->releasebuffers_called = 0;
#endif
    
    DebugError_Init(&o->d_err, o->pg);
    DebugObject_Init(&o->d_obj);
}

void BSSLConnection_Free (BSSLConnection *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
#ifndef NDEBUG
    ASSERT(o->releasebuffers_called || !o->user_io_started)
#endif
    ASSERT(o->backend->threadwork_state == THREADWORK_STATE_NONE)
    
    if (o->up) {
        // free recv job
        BPending_Free(&o->recv_job);
        
        // free recv interface
        StreamRecvInterface_Free(&o->recv_if);
        
        // free send interface
        StreamPassInterface_Free(&o->send_if);
    }
    
    // free init job
    BPending_Free(&o->init_job);
    
    // unset backend connection
    o->backend->con = NULL;
}

void BSSLConnection_ReleaseBuffers (BSSLConnection *o)
{
    DebugObject_Access(&o->d_obj);
#ifndef NDEBUG
    ASSERT(!o->releasebuffers_called)
#endif
    
    // wait for threadwork to finish
    if (o->backend->threadwork_state != THREADWORK_STATE_NONE) {
        BThreadWork_Free(&o->backend->threadwork);
        o->backend->threadwork_state = THREADWORK_STATE_NONE;
    }
    
#ifndef NDEBUG
    o->releasebuffers_called = 1;
#endif
}

StreamPassInterface * BSSLConnection_GetSendIf (BSSLConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->up)
    
    return &o->send_if;
}

StreamRecvInterface * BSSLConnection_GetRecvIf (BSSLConnection *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->up)
    
    return &o->recv_if;
}
