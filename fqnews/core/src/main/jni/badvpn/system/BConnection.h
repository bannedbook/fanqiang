/**
 * @file BConnection.h
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

#ifndef BADVPN_SYSTEM_BCONNECTION
#define BADVPN_SYSTEM_BCONNECTION

#include <misc/debug.h>
#include <flow/StreamPassInterface.h>
#include <flow/StreamRecvInterface.h>
#include <system/BAddr.h>
#include <system/BReactor.h>
#include <system/BNetwork.h>



/**
 * Checks if the given address is supported by {@link BConnection} and related objects.
 * 
 * @param addr address to check. Must be a proper {@link BAddr} object according to
 *             {@link BIPAddr_Assert}.
 * @return 1 if supported, 0 if not
 */
int BConnection_AddressSupported (BAddr addr);


#define BLISCON_FROM_ADDR 1
#define BLISCON_FROM_UNIX 2

struct BLisCon_from {
    int type;
    union {
        struct {
            BAddr addr;
        } from_addr;
#ifndef BADVPN_USE_WINAPI
        struct {
            char const *socket_path;
        } from_unix;
#endif
    } u;
};

static struct BLisCon_from BLisCon_from_addr (BAddr addr)
{
    struct BLisCon_from res;
    res.type = BLISCON_FROM_ADDR;
    res.u.from_addr.addr = addr;
    return res;
}

#ifndef BADVPN_USE_WINAPI
static struct BLisCon_from BLisCon_from_unix (char const *socket_path)
{
    struct BLisCon_from res;
    res.type = BLISCON_FROM_UNIX;
    res.u.from_unix.socket_path = socket_path;
    return res;
}
#endif


struct BListener_s;

/**
 * Object which listens for connections on an address.
 * When a connection is ready, the {@link BListener_handler} handler is called, from which
 * the connection can be accepted into a new {@link BConnection} object.
 */
typedef struct BListener_s BListener;

/**
 * Handler called when a new connection is ready.
 * The connection can be accepted by calling {@link BConnection_Init} with the a
 * BCONNECTION_SOURCE_LISTENER 'source' argument.
 * If no attempt is made to accept the connection from the job closure of this handler,
 * the connection will be discarded.
 * 
 * @param user as in {@link BListener_Init}
 */
typedef void (*BListener_handler) (void *user);

/**
 * Common listener initialization function.
 * 
 * The other type-specific init functions are wrappers around this one.
 */
int BListener_InitFrom (BListener *o, struct BLisCon_from from,
                        BReactor *reactor, void *user,
                        BListener_handler handler) WARN_UNUSED;

/**
 * Initializes the object for listening on an address.
 * {@link BNetwork_GlobalInit} must have been done.
 * 
 * @param o the object
 * @param addr address to listen on
 * @param reactor reactor we live in
 * @param user argument to handler
 * @param handler handler called when a connection can be accepted
 * @return 1 on success, 0 on failure
 */
int BListener_Init (BListener *o, BAddr addr, BReactor *reactor, void *user,
                    BListener_handler handler) WARN_UNUSED;

#ifndef BADVPN_USE_WINAPI
/**
 * Initializes the object for listening on a Unix socket.
 * {@link BNetwork_GlobalInit} must have been done.
 * 
 * @param o the object
 * @param socket_path socket path for listening
 * @param reactor reactor we live in
 * @param user argument to handler
 * @param handler handler called when a connection can be accepted
 * @return 1 on success, 0 on failure
 */
int BListener_InitUnix (BListener *o, const char *socket_path, BReactor *reactor, void *user,
                        BListener_handler handler) WARN_UNUSED;
#endif

/**
 * Frees the object.
 * 
 * @param o the object
 */
void BListener_Free (BListener *o);



struct BConnector_s;

/**
 * Object which connects to an address.
 * When the connection attempt finishes, the {@link BConnector_handler} handler is called, from which,
 * if successful, the resulting connection can be moved to a new {@link BConnection} object.
 */
typedef struct BConnector_s BConnector;

/**
 * Handler called when the connection attempt finishes.
 * If the connection attempt succeeded (is_error==0), the new connection can be used by calling
 * {@link BConnection_Init} with a BCONNECTION_SOURCE_TYPE_CONNECTOR 'source' argument.
 * This handler will be called at most once. The connector object need not be freed after it
 * is called. 
 * 
 * @param user as in {@link BConnector_Init}
 * @param is_error whether the connection attempt succeeded (0) or failed (1)
 */
typedef void (*BConnector_handler) (void *user, int is_error);

/**
 * Common connector initialization function.
 * 
 * The other type-specific init functions are wrappers around this one.
 */
int BConnector_InitFrom (BConnector *o, struct BLisCon_from from, BReactor *reactor, void *user,
                         BConnector_handler handler) WARN_UNUSED;

/**
 * Initializes the object for connecting to an address.
 * {@link BNetwork_GlobalInit} must have been done.
 * 
 * @param o the object
 * @param addr address to connect to
 * @param reactor reactor we live in
 * @param user argument to handler
 * @param handler handler called when the connection attempt finishes
 * @return 1 on success, 0 on failure
 */
int BConnector_Init (BConnector *o, BAddr addr, BReactor *reactor, void *user,
                     BConnector_handler handler) WARN_UNUSED;

#ifndef BADVPN_USE_WINAPI
/**
 * Initializes the object for connecting to a Unix socket.
 * {@link BNetwork_GlobalInit} must have been done.
 * 
 * @param o the object
 * @param socket_path socket path for connecting
 * @param reactor reactor we live in
 * @param user argument to handler
 * @param handler handler called when the connection attempt finishes
 * @return 1 on success, 0 on failure
 */
int BConnector_InitUnix (BConnector *o, const char *socket_path, BReactor *reactor, void *user,
                         BConnector_handler handler) WARN_UNUSED;
#endif

/**
 * Frees the object.
 * 
 * @param o the object
 */
void BConnector_Free (BConnector *o);



#define BCONNECTION_SOURCE_TYPE_LISTENER 1
#define BCONNECTION_SOURCE_TYPE_CONNECTOR 2
#define BCONNECTION_SOURCE_TYPE_PIPE 3

struct BConnection_source {
    int type;
    union {
        struct {
            BListener *listener;
            BAddr *out_addr;
        } listener;
        struct {
            BConnector *connector;
        } connector;
#ifndef BADVPN_USE_WINAPI
        struct {
            int pipefd;
            int close_it;
        } pipe;
#endif
    } u;
};

static struct BConnection_source BConnection_source_listener (BListener *listener, BAddr *out_addr)
{
    struct BConnection_source s;
    s.type = BCONNECTION_SOURCE_TYPE_LISTENER;
    s.u.listener.listener = listener;
    s.u.listener.out_addr = out_addr;
    return s;
}

static struct BConnection_source BConnection_source_connector (BConnector *connector)
{
    struct BConnection_source s;
    s.type = BCONNECTION_SOURCE_TYPE_CONNECTOR;
    s.u.connector.connector = connector;
    return s;
}

#ifndef BADVPN_USE_WINAPI
static struct BConnection_source BConnection_source_pipe (int pipefd, int close_it)
{
    struct BConnection_source s;
    s.type = BCONNECTION_SOURCE_TYPE_PIPE;
    s.u.pipe.pipefd = pipefd;
    s.u.pipe.close_it = close_it;
    return s;
}
#endif

struct BConnection_s;

/**
 * Object which represents a stream connection. This is usually a TCP connection, either client
 * or server, but may also be used with any file descriptor (e.g. pipe) on Unix-like systems.
 * Sending and receiving is performed via {@link StreamPassInterface} and {@link StreamRecvInterface},
 * respectively.
 */
typedef struct BConnection_s BConnection;

#define BCONNECTION_EVENT_ERROR 1
#define BCONNECTION_EVENT_RECVCLOSED 2

/**
 * Handler called when an error occurs or the receive end of the connection was closed
 * by the remote peer.
 * - If event is BCONNECTION_EVENT_ERROR, the connection is no longer usable and must be freed
 *   from withing the job closure of this handler. No further I/O or interface initialization
 *   must occur.
 * - If event is BCONNECTION_EVENT_RECVCLOSED, no further receive I/O or receive interface
 *   initialization must occur. It is guarantted that the receive interface was initialized.
 * 
 * @param user as in {@link BConnection_Init} or {@link BConnection_SetHandlers}
 * @param event what happened - BCONNECTION_EVENT_ERROR or BCONNECTION_EVENT_RECVCLOSED
 */
typedef void (*BConnection_handler) (void *user, int event);

/**
 * Initializes the object.
 * {@link BNetwork_GlobalInit} must have been done.
 * 
 * @param o the object
 * @param source specifies what the connection comes from. This argument must be created with one of the
 *               following macros:
 *               - BCONNECTION_SOURCE_LISTENER(BListener *, BAddr *)
 *                 Accepts a connection ready on a {@link BListener} object. Must be called from the job
 *                 closure of the listener's {@link BListener_handler}, and must be the first attempt
 *                 for this handler invocation. The address of the client is written if the address
 *                 argument is not NULL (theoretically an invalid address may be returned).
 *               - BCONNECTION_SOURCE_CONNECTOR(Bconnector *)
 *                 Uses a connection establised with {@link BConnector}. Must be called from the job
 *                 closure of the connector's {@link BConnector_handler}, the handler must be reporting
 *                 successful connection, and must be the first attempt for this handler invocation.
 *               - BCONNECTION_SOURCE_PIPE(int pipefd, int close_it)
 *                 On Unix-like systems, uses the provided file descriptor. The file descriptor number must
 *                 be >=0. If close_it is true, the connector will take responsibility of closing the
 *                 pipefd. Note that it will be closed even when this Init fails.
 * @param reactor reactor we live in
 * @param user argument to handler
 * @param handler handler called when an error occurs or the receive end of the connection was closed
 *                by the remote peer.
 * @return 1 on success, 0 on failure
 */
int BConnection_Init (BConnection *o, struct BConnection_source source, BReactor *reactor, void *user,
                      BConnection_handler handler) WARN_UNUSED;

/**
 * Frees the object.
 * The send and receive interfaces must not be initialized.
 * If the connection was created with a BCONNECTION_SOURCE_PIPE 'source' argument, the file descriptor
 * will not be closed.
 * 
 * @param o the object
 */
void BConnection_Free (BConnection *o);

/**
 * Updates the handler function.
 * 
 * @param o the object
 * @param user argument to handler
 * @param handler new handler function, as in {@link BConnection_Init}. Additionally, may be NULL to
 *                remove the current handler. In this case, a proper handler must be set before anything
 *                can happen with the connection. This is used when moving the connection ownership from
 *                one module to another.
 */
void BConnection_SetHandlers (BConnection *o, void *user, BConnection_handler handler);

/**
 * Sets the SO_SNDBUF socket option.
 * 
 * @param o the object
 * @param buf_size value for SO_SNDBUF option
 * @return 1 on success, 0 on failure
 */
int BConnection_SetSendBuffer (BConnection *o, int buf_size);

/**
 * Initializes the send interface for the connection.
 * The send interface must not be initialized.
 * 
 * @param o the object
 */
void BConnection_SendAsync_Init (BConnection *o);

/**
 * Frees the send interface for the connection.
 * The send interface must be initialized.
 * If the send interface was busy when this is called, the connection is no longer usable and must be
 * freed before any further I/O or interface initialization.
 * 
 * @param o the object
 */
void BConnection_SendAsync_Free (BConnection *o);

/**
 * Returns the send interface.
 * The send interface must be initialized.
 * 
 * @param o the object
 * @return send interface
 */
StreamPassInterface * BConnection_SendAsync_GetIf (BConnection *o);

/**
 * Initializes the receive interface for the connection.
 * The receive interface must not be initialized.
 * 
 * @param o the object
 */
void BConnection_RecvAsync_Init (BConnection *o);

/**
 * Frees the receive interface for the connection.
 * The receive interface must be initialized.
 * If the receive interface was busy when this is called, the connection is no longer usable and must be
 * freed before any further I/O or interface initialization.
 * 
 * @param o the object
 */
void BConnection_RecvAsync_Free (BConnection *o);

/**
 * Returns the receive interface.
 * The receive interface must be initialized.
 * 
 * @param o the object
 * @return receive interface
 */
StreamRecvInterface * BConnection_RecvAsync_GetIf (BConnection *o);



#ifdef BADVPN_USE_WINAPI
#include "BConnection_win.h"
#else
#include "BConnection_unix.h"
#endif

#endif
