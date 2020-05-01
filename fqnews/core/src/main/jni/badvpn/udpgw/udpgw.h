/*
 * Copyright (C) Ambroz Bizjak <ambrop7@gmail.com>
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

// name of the program
#define PROGRAM_NAME "udpgw"

// maxiumum listen addresses
#define MAX_LISTEN_ADDRS 16

// maximum datagram size
#define DEFAULT_UDP_MTU 65520

// connection buffer size for sending to client, in packets
#define CONNECTION_CLIENT_BUFFER_SIZE 1

// connection buffer size for sending to UDP, in packets
#define CONNECTION_UDP_BUFFER_SIZE 1

// maximum number of clients
#define DEFAULT_MAX_CLIENTS 3

// maximum connections for client
#define DEFAULT_MAX_CONNECTIONS_FOR_CLIENT 256

// how long after nothing has been received to disconnect a client
#define CLIENT_DISCONNECT_TIMEOUT 20000

// SO_SNDBFUF socket option for clients, 0 to not set
#define CLIENT_DEFAULT_SOCKET_SEND_BUFFER 1048576
