/**
 * @file PasswordListener.h
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
 * Object used to listen on a socket, accept clients and identify them
 * based on a number they send.
 */

#ifndef BADVPN_CLIENT_PASSWORDLISTENER_H
#define BADVPN_CLIENT_PASSWORDLISTENER_H

#include <stdint.h>

#include <prio.h>

#include <nss/cert.h>
#include <nss/keyhi.h>

#include <misc/debug.h>
#include <misc/sslsocket.h>
#include <structure/LinkedList1.h>
#include <structure/BAVL.h>
#include <base/DebugObject.h>
#include <flow/SingleStreamReceiver.h>
#include <system/BConnection.h>
#include <nspr_support/BSSLConnection.h>

/**
 * Handler function called when a client identifies itself with a password
 * belonging to one of the password entries.
 * The password entry is unregistered before the handler is called
 * and must not be unregistered again.
 * 
 * @param user as in {@link PasswordListener_AddEntry}
 * @param sock structure containing a {@link BConnection} and, if TLS is enabled,
 *             the SSL socket with the bottom layer connected to the async interfaces
 *             of the {@link BConnection} object. The structure was allocated with
 *             malloc() and the user is responsible for freeing it.
 */
typedef void (*PasswordListener_handler_client) (void *user, sslsocket *sock);

struct PasswordListenerClient;

/**
 * Object used to listen on a socket, accept clients and identify them
 * based on a number they send.
 */
typedef struct {
    BReactor *bsys;
    BThreadWorkDispatcher *twd;
    int ssl;
    int ssl_flags;
    PRFileDesc model_dprfd;
    PRFileDesc *model_prfd;
    struct PasswordListenerClient *clients_data;
    LinkedList1 clients_free;
    LinkedList1 clients_used;
    BAVL passwords;
    BListener listener;
    DebugObject d_obj;
} PasswordListener;

typedef struct {
    uint64_t password;
    BAVLNode tree_node;
    PasswordListener_handler_client handler_client;
    void *user;
} PasswordListener_pwentry;

struct PasswordListenerClient {
    PasswordListener *l;
    LinkedList1Node list_node;
    sslsocket *sock;
    BSSLConnection sslcon;
    SingleStreamReceiver receiver;
    uint64_t recv_buffer;
};

/**
 * Initializes the object.
 * 
 * @param l the object
 * @param bsys reactor we live in
 * @param twd thread work dispatcher. May be NULL if ssl_flags does not request performing SSL
 *            operations in threads.
 * @param listen_addr address to listen on. Must be supported according to {@link BConnection_AddressSupported}.
 * @param max_clients maximum number of client to hold until they are identified.
 *                    Must be >0.
 * @param ssl whether to use TLS. Must be 1 or 0.
 * @param ssl_flags flags passed down to {@link BSSLConnection_MakeBackend}. May be used to
 *                  request performing SSL operations in threads.
 * @param cert if using TLS, the server certificate
 * @param key if using TLS, the private key
 * @return 1 on success, 0 on failure
 */
int PasswordListener_Init (PasswordListener *l, BReactor *bsys, BThreadWorkDispatcher *twd, BAddr listen_addr, int max_clients, int ssl, int ssl_flags, CERTCertificate *cert, SECKEYPrivateKey *key) WARN_UNUSED;

/**
 * Frees the object.
 * 
 * @param l the object
 */
void PasswordListener_Free (PasswordListener *l);

/**
 * Registers a password entry.
 * 
 * @param l the object
 * @param entry uninitialized entry structure
 * @param handler_client handler function to call when a client identifies
 *                       with the password which this function returns
 * @param user value to pass to handler function
 * @return password which a client should send to be recognized and
 *         dispatched to the handler function. Should be treated as a numeric
 *         value, which a client should as a little-endian 64-bit unsigned integer
 *         when it connects.
 */
uint64_t PasswordListener_AddEntry (PasswordListener *l, PasswordListener_pwentry *entry, PasswordListener_handler_client handler_client, void *user);

/**
 * Unregisters a password entry.
 * Note that when a client is dispatched, its entry is unregistered
 * automatically and must not be unregistered again here.
 * 
 * @param l the object
 * @param entry entry to unregister
 */
void PasswordListener_RemoveEntry (PasswordListener *l, PasswordListener_pwentry *entry);

#endif
