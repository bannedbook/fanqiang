/**
 * @file BDatagram.h
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

#ifndef BADVPN_SYSTEM_BDATAGRAM
#define BADVPN_SYSTEM_BDATAGRAM

#include <misc/debug.h>
#include <flow/PacketPassInterface.h>
#include <flow/PacketRecvInterface.h>
#include <system/BAddr.h>
#include <system/BReactor.h>
#include <system/BNetwork.h>

struct BDatagram_s;

/**
 * Represents datagram communication. This is usually UDP, but may also be Linux packet sockets.
 * Sending and receiving is performed via {@link PacketPassInterface} and {@link PacketRecvInterface},
 * respectively.
 */
typedef struct BDatagram_s BDatagram;

#define BDATAGRAM_EVENT_ERROR 1

/**
 * Handler called when an error occurs with the datagram object.
 * The datagram object is no longer usable and must be freed from withing the job closure of
 * this handler. No further I/O, interface initialization, binding and send address setting
 * must occur.
 * 
 * @param user as in {@link BDatagram_Init}
 * @param event always BDATAGRAM_EVENT_ERROR
 */
typedef void (*BDatagram_handler) (void *user, int event);

/**
 * Checks if the given address family (from {@link BAddr.h}) is supported by {@link BDatagram}
 * and related objects.
 * 
 * @param family family to check
 * @return 1 if supported, 0 if not
 */
int BDatagram_AddressFamilySupported (int family);

/**
 * Initializes the object.
 * {@link BNetwork_GlobalInit} must have been done.
 * 
 * @param o the object
 * @param family address family. Must be supported according to {@link BDatagram_AddressFamilySupported}.
 * @param reactor reactor we live in
 * @param user argument to handler
 * @param handler handler called when an error occurs
 * @return 1 on success, 0 on failure
 */
int BDatagram_Init (BDatagram *o, int family, BReactor *reactor, void *user,
                    BDatagram_handler handler) WARN_UNUSED;

/**
 * Frees the object.
 * The send and receive interfaces must not be initialized.
 * 
 * @param o the object
 */
void BDatagram_Free (BDatagram *o);

/**
 * Binds to the given local address.
 * May initiate I/O.
 * 
 * @param o the object
 * @param addr address to bind to. Its family must be supported according to {@link BDatagram_AddressFamilySupported}.
 * @return 1 on success, 0 on failure
 */
int BDatagram_Bind (BDatagram *o, BAddr addr) WARN_UNUSED;

/**
 * Sets addresses for sending.
 * May initiate I/O.
 * 
 * @param o the object
 * @param remote_addr destination address for sending datagrams. Its family must be supported according
 *                    to {@link BDatagram_AddressFamilySupported}.
 * @param local_addr local source IP address. May be an invalid address, otherwise its family must be
 *                   supported according to {@link BDatagram_AddressFamilySupported}.
 */
void BDatagram_SetSendAddrs (BDatagram *o, BAddr remote_addr, BIPAddr local_addr);

/**
 * Returns the remote and local address of the last datagram received.
 * Fails if and only if no datagrams have been received yet.
 * 
 * @param o the object
 * @param remote_addr returns the remote source address of the datagram. May be an invalid address, theoretically.
 * @param local_addr returns the local destination IP address. May be an invalid address.
 * @return 1 on success, 0 on failure
 */
int BDatagram_GetLastReceiveAddrs (BDatagram *o, BAddr *remote_addr, BIPAddr *local_addr);

#ifndef BADVPN_USE_WINAPI
/**
 * Returns the underlying socket file descriptor of the datagram object.
 * Available on Unix-like systems only.
 * 
 * @param o the object
 * @return file descriptor
 */
int BDatagram_GetFd (BDatagram *o);
#endif

/**
 * Sets the SO_REUSEADDR option for the underlying socket.
 * 
 * @param o the object
 * @param reuse value of the option. Must be 0 or 1.
 */
int BDatagram_SetReuseAddr (BDatagram *o, int reuse);

/**
 * Initializes the send interface.
 * The send interface must not be initialized.
 * 
 * @param o the object
 * @param mtu maximum transmission unit. Must be >=0.
 */
void BDatagram_SendAsync_Init (BDatagram *o, int mtu);

/**
 * Frees the send interface.
 * The send interface must be initialized.
 * If the send interface was busy when this is called, the datagram object is no longer usable and must be
 * freed before any further I/O or interface initialization.
 * 
 * @param o the object
 */
void BDatagram_SendAsync_Free (BDatagram *o);

/**
 * Returns the send interface.
 * The send interface must be initialized.
 * The MTU of the interface will be as in {@link BDatagram_SendAsync_Init}.
 * 
 * @param o the object
 * @return send interface
 */
PacketPassInterface * BDatagram_SendAsync_GetIf (BDatagram *o);

/**
 * Initializes the receive interface.
 * The receive interface must not be initialized.
 * 
 * @param o the object
 * @param mtu maximum transmission unit. Must be >=0.
 */
void BDatagram_RecvAsync_Init (BDatagram *o, int mtu);

/**
 * Frees the receive interface.
 * The receive interface must be initialized.
 * If the receive interface was busy when this is called, the datagram object is no longer usable and must be
 * freed before any further I/O or interface initialization.
 * 
 * @param o the object
 */
void BDatagram_RecvAsync_Free (BDatagram *o);

/**
 * Returns the receive interface.
 * The receive interface must be initialized.
 * The MTU of the interface will be as in {@link BDatagram_RecvAsync_Init}.
 * 
 * @param o the object
 * @return receive interface
 */
PacketRecvInterface * BDatagram_RecvAsync_GetIf (BDatagram *o);

#ifdef BADVPN_USE_WINAPI
#include "BDatagram_win.h"
#else
#include "BDatagram_unix.h"
#endif

#endif
