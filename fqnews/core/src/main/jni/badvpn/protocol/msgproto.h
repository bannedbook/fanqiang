/**
 * @file msgproto.h
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
 * MsgProto is used by each pair of VPN peers as messages through the server, in order to
 * establish a direct data connection. MsgProto operates on top of the SCProto message
 * service, optionally secured with SSL; see {@link scproto.h} for details.
 * 
 * MsgProto is built with BProto, the protocol and code generator for building
 * custom message protocols. The BProto specification file is msgproto.bproto.
 * 
 * It goes roughly like that:
 * 
 * We name one peer the master and the other the slave. The master is the one with
 * greater ID.
 * When the peers get to know about each other, the master starts the binding procedure.
 * It binds/listens to an address, and sends the slave the "youconnect" message. It
 * contains a list of external addresses for that bind address and additional parameters.
 * Each external address includes a string called a scope name. The slave, which receives
 * the "youconnect" message, finds the first external address whose scope it recognizes,
 * and attempts to establish connection to that address. If it finds an address, buf fails
 * at connecting, it sends "youretry", which makes the master restart the binding procedure
 * after some time. If it however does not recognize any external address, it sends
 * "cannotconnect" back to the master.
 * When the master receives the "cannotconnect", it tries the next bind address, as described
 * above. When the master runs out of bind addresses, it sends "cannotbind" to the slave.
 * When the slave receives the "cannotbind", it starts its own binding procedure, similarly
 * to what is described above, with master and slave reversed. First difference is if the
 * master fails to connect to a recognized address, it doesn't send "youretry", but rather
 * simply restarts the whole procedure after some time. The other difference is when the
 * slave runs out of bind addresses, it not only sends "cannotbind" to the master, but
 * registers relaying to the master. And in this case, when the master receives the "cannotbind",
 * it doesn't start the binding procedure all all over, but registers relaying to the slave.
 */

#ifndef BADVPN_PROTOCOL_MSGPROTO_H
#define BADVPN_PROTOCOL_MSGPROTO_H

#include <generated/bproto_msgproto.h>

#define MSGID_YOUCONNECT 1
#define MSGID_CANNOTCONNECT 2
#define MSGID_CANNOTBIND 3
#define MSGID_YOURETRY 5
#define MSGID_SEED 6
#define MSGID_CONFIRMSEED 7

#define MSG_MAX_PAYLOAD (SC_MAX_MSGLEN - msg_SIZEtype - msg_SIZEpayload(0))

#endif
