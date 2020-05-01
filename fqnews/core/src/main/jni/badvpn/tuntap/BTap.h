/**
 * @file BTap.h
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
 * TAP device abstraction.
 */

#ifndef BADVPN_TUNTAP_BTAP_H
#define BADVPN_TUNTAP_BTAP_H

#if (defined(BADVPN_USE_WINAPI) + defined(BADVPN_LINUX) + defined(BADVPN_FREEBSD)) != 1
#error Unknown TAP backend or too many TAP backends
#endif

#include <stdint.h>

#ifdef BADVPN_USE_WINAPI
#else
#include <net/if.h>
#endif

#include <misc/debug.h>
#include <misc/debugerror.h>
#include <base/DebugObject.h>
#include <system/BReactor.h>
#include <flow/PacketRecvInterface.h>

#define BTAP_ETHERNET_HEADER_LENGTH 14

/**
 * Handler called when an error occurs on the device.
 * The object must be destroyed from the job context of this
 * handler, and no further I/O may occur.
 * 
 * @param user as in {@link BTap_Init}
 */
typedef void (*BTap_handler_error) (void *used);

typedef struct {
    BReactor *reactor;
    BTap_handler_error handler_error;
    void *handler_error_user;
    int frame_mtu;
    PacketRecvInterface output;
    uint8_t *output_packet;
    
#ifdef BADVPN_USE_WINAPI
    HANDLE device;
    BReactorIOCPOverlapped send_olap;
    BReactorIOCPOverlapped recv_olap;
#else
    int close_fd;
    int fd;
    BFileDescriptor bfd;
    int poll_events;
#endif
    
    DebugError d_err;
    DebugObject d_obj;
} BTap;

/**
 * Initializes the TAP device.
 *
 * @param o the object
 * @param BReactor {@link BReactor} we live in
 * @param devname name of the devece to open.
 *                On Linux: a network interface name. If it is NULL, no
 *                specific device will be requested, and the operating system
 *                may create a new device.
 *                On Windows: a string "component_id:device_name", where
 *                component_id is a string identifying the driver, and device_name
 *                is the name of the network interface. If component_id is empty,
 *                a hardcoded default will be used instead. If device_name is empty,
 *                the first device found with a matching component_id will be used.
 *                Specifying a NULL devname is equivalent to specifying ":".
 * @param handler_error error handler function
 * @param handler_error_user value passed to error handler
 * @param tun whether to create a TUN (IP) device or a TAP (Ethernet) device. Must be 0 or 1.
 * @return 1 on success, 0 on failure
 */
int BTap_Init (BTap *o, BReactor *bsys, char *devname, BTap_handler_error handler_error, void *handler_error_user, int tun) WARN_UNUSED;

enum BTap_dev_type {BTAP_DEV_TUN, BTAP_DEV_TAP};

enum BTap_init_type {
    BTAP_INIT_STRING,
#ifndef BADVPN_USE_WINAPI
    BTAP_INIT_FD,
#endif
};

struct BTap_init_data {
    enum BTap_dev_type dev_type;
    enum BTap_init_type init_type;
    union {
        char *string;
        struct {
            int fd;
            int mtu;
        } fd;
    } init;
};

/**
 * Initializes the TAP device.
 *
 * @param o the object
 * @param BReactor {@link BReactor} we live in
 * @param init_data struct containing initialization parameters (to allow transparent passing).
 *                  init.data.dev_type must be either BTAP_DEV_TUN for an IP device, or
 *                  BTAP_DEV_TAP for an Ethernet device.
 *                  init_data.init_type must be either BTAP_INIT_STRING or BTAP_INIT_FD.
 *                  For BTAP_INIT_STRING, init_data.init.string specifies the TUN or TAP
 *                  device, as described next.
 *                  On Linux: a network interface name. If it is NULL, no
 *                  specific device will be requested, and the operating system
 *                  may create a new device.
 *                  On Windows: a string "component_id:device_name", where
 *                  component_id is a string identifying the driver, and device_name
 *                  is the name of the network interface. If component_id is empty,
 *                  a hardcoded default will be used instead. If device_name is empty,
 *                  the first device found with a matching component_id will be used.
 *                  Specifying NULL is equivalent to specifying ":".
 *                  For BTAP_INIT_FD, the device is initialized using a file descriptor.
 *                  In this case, init_data.init.fd.fd must be set to the file descriptor,
 *                  and init_data.init.fd.mtu must be set to the largest IP packet or
 *                  Ethernet frame supported, for a TUN or TAP device, respectively.
 *                  File descriptor initialization is not supported on Windows.
 * @param handler_error error handler function
 * @param handler_error_user value passed to error handler
 * @return 1 on success, 0 on failure
 */
int BTap_Init2 (BTap *o, BReactor *reactor, struct BTap_init_data init_data, BTap_handler_error handler_error, void *handler_error_user) WARN_UNUSED;

/**
 * Frees the TAP device.
 *
 * @param o the object
 */
void BTap_Free (BTap *o);

/**
 * Returns the device's maximum transmission unit (including any protocol headers).
 *
 * @param o the object
 * @return device's MTU
 */
int BTap_GetMTU (BTap *o);

/**
 * Sends a packet to the device.
 * Any errors will be reported via a job.
 * 
 * @param o the object
 * @param data packet to send
 * @param data_len length of packet. Must be >=0 and <=MTU, as reported by {@link BTap_GetMTU}.
 */
void BTap_Send (BTap *o, uint8_t *data, int data_len);

/**
 * Returns a {@link PacketRecvInterface} for reading packets from the device.
 * The MTU of the interface will be {@link BTap_GetMTU}.
 * 
 * @param o the object
 * @return output interface
 */
PacketRecvInterface * BTap_GetOutput (BTap *o);

#endif
