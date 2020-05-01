/**
 * @file BTap.c
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
#include <stdio.h>

#ifdef BADVPN_USE_WINAPI
    #include <windows.h>
    #include <winioctl.h>
    #include <objbase.h>
    #include <wtypes.h>
    #include "wintap-common.h"
    #include <tuntap/tapwin32-funcs.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
    #include <sys/ioctl.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/socket.h>
    #include <net/if.h>
    #include <net/if_arp.h>
    #ifdef BADVPN_LINUX
        #include <linux/if_tun.h>
    #endif
    #ifdef BADVPN_FREEBSD
        #include <net/if_tun.h>
        #include <net/if_tap.h>
    #endif
#endif

#include <base/BLog.h>

#include <tuntap/BTap.h>

#include <generated/blog_channel_BTap.h>

static void report_error (BTap *o);
static void output_handler_recv (BTap *o, uint8_t *data);

#ifdef BADVPN_USE_WINAPI

static void recv_olap_handler (BTap *o, int event, DWORD bytes)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->output_packet)
    ASSERT(event == BREACTOR_IOCP_EVENT_SUCCEEDED || event == BREACTOR_IOCP_EVENT_FAILED)
    
    // set no output packet
    o->output_packet = NULL;
    
    if (event == BREACTOR_IOCP_EVENT_FAILED) {
        BLog(BLOG_ERROR, "read operation failed");
        report_error(o);
        return;
    }
    
    ASSERT(bytes >= 0)
    ASSERT(bytes <= o->frame_mtu)
    
    // done
    PacketRecvInterface_Done(&o->output, bytes);
}

#else

static void fd_handler (BTap *o, int events)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    
    if (events&(BREACTOR_ERROR|BREACTOR_HUP)) {
        BLog(BLOG_WARNING, "device fd reports error?");
    }
    
    if (events&BREACTOR_READ) do {
        ASSERT(o->output_packet)
        
        // try reading into the buffer
        int bytes = read(o->fd, o->output_packet, o->frame_mtu);
        if (bytes <= 0) {
            // Treat zero return value the same as EAGAIN.
            // See: https://bugzilla.kernel.org/show_bug.cgi?id=96381
            if (bytes == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
                // retry later
                break;
            }
            // report fatal error
            report_error(o);
            return;
        }
        
        ASSERT_FORCE(bytes <= o->frame_mtu)
        
        // set no output packet
        o->output_packet = NULL;
        
        // update events
        o->poll_events &= ~BREACTOR_READ;
        BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->poll_events);
        
        // inform receiver we finished the packet
        PacketRecvInterface_Done(&o->output, bytes);
    } while (0);
}

#endif

void report_error (BTap *o)
{
    DEBUGERROR(&o->d_err, o->handler_error(o->handler_error_user));
}

void output_handler_recv (BTap *o, uint8_t *data)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(data)
    ASSERT(!o->output_packet)
    
#ifdef BADVPN_USE_WINAPI
    
    memset(&o->recv_olap.olap, 0, sizeof(o->recv_olap.olap));
    
    // read
    BOOL res = ReadFile(o->device, data, o->frame_mtu, NULL, &o->recv_olap.olap);
    if (res == FALSE && GetLastError() != ERROR_IO_PENDING) {
        BLog(BLOG_ERROR, "ReadFile failed (%u)", GetLastError());
        report_error(o);
        return;
    }
    
    o->output_packet = data;
    
#else
    
    // attempt read
    int bytes = read(o->fd, data, o->frame_mtu);
    if (bytes <= 0) {
        if (bytes == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
            // See note about zero return in fd_handler.
            // retry later in fd_handler
            // remember packet
            o->output_packet = data;
            // update events
            o->poll_events |= BREACTOR_READ;
            BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, o->poll_events);
            return;
        }
        // report fatal error
        report_error(o);
        return;
    }
    
    ASSERT_FORCE(bytes <= o->frame_mtu)
    
    PacketRecvInterface_Done(&o->output, bytes);
    
#endif
}

int BTap_Init (BTap *o, BReactor *reactor, char *devname, BTap_handler_error handler_error, void *handler_error_user, int tun)
{
    ASSERT(tun == 0 || tun == 1)
    
    struct BTap_init_data init_data;
    init_data.dev_type = tun ? BTAP_DEV_TUN : BTAP_DEV_TAP;
    init_data.init_type = BTAP_INIT_STRING;
    init_data.init.string = devname;
    
    return BTap_Init2(o, reactor, init_data, handler_error, handler_error_user);
}

int BTap_Init2 (BTap *o, BReactor *reactor, struct BTap_init_data init_data, BTap_handler_error handler_error, void *handler_error_user)
{
    ASSERT(init_data.dev_type == BTAP_DEV_TUN || init_data.dev_type == BTAP_DEV_TAP)
    
    // init arguments
    o->reactor = reactor;
    o->handler_error = handler_error;
    o->handler_error_user = handler_error_user;
    
    #ifdef BADVPN_USE_WINAPI
    
    ASSERT(init_data.init_type == BTAP_INIT_STRING)
    
    // parse device specification
    
    if (!init_data.init.string) {
        BLog(BLOG_ERROR, "no device specification provided");
        goto fail0;
    }
    
    char *device_component_id;
    char *device_name;
    uint32_t tun_addrs[3];
    
    if (init_data.dev_type == BTAP_DEV_TUN) {
        if (!tapwin32_parse_tun_spec(init_data.init.string, &device_component_id, &device_name, tun_addrs)) {
            BLog(BLOG_ERROR, "failed to parse TUN device specification");
            goto fail0;
        }
    } else {
        if (!tapwin32_parse_tap_spec(init_data.init.string, &device_component_id, &device_name)) {
            BLog(BLOG_ERROR, "failed to parse TAP device specification");
            goto fail0;
        }
    }
    
    // locate device path
    
    char device_path[TAPWIN32_MAX_REG_SIZE];
    
    BLog(BLOG_INFO, "Looking for TAP-Win32 with component ID %s, name %s", device_component_id, device_name);
    
    if (!tapwin32_find_device(device_component_id, device_name, &device_path)) {
        BLog(BLOG_ERROR, "Could not find device");
        goto fail1;
    }
    
    // open device
    
    BLog(BLOG_INFO, "Opening device %s", device_path);
    
    o->device = CreateFile(device_path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM|FILE_FLAG_OVERLAPPED, 0);
    if (o->device == INVALID_HANDLE_VALUE) {
        BLog(BLOG_ERROR, "CreateFile failed");
        goto fail1;
    }
    
    // set TUN if needed
    
    DWORD len;
    
    if (init_data.dev_type == BTAP_DEV_TUN) {
        if (!DeviceIoControl(o->device, TAP_IOCTL_CONFIG_TUN, tun_addrs, sizeof(tun_addrs), tun_addrs, sizeof(tun_addrs), &len, NULL)) {
            BLog(BLOG_ERROR, "DeviceIoControl(TAP_IOCTL_CONFIG_TUN) failed");
            goto fail2;
        }
    }
    
    // get MTU
    
    ULONG umtu = 0;
    
    if (!DeviceIoControl(o->device, TAP_IOCTL_GET_MTU, &umtu, sizeof(umtu), &umtu, sizeof(umtu), &len, NULL)) {
        BLog(BLOG_ERROR, "DeviceIoControl(TAP_IOCTL_GET_MTU) failed");
        goto fail2;
    }
    
    if (init_data.dev_type == BTAP_DEV_TUN) {
        o->frame_mtu = umtu;
    } else {
        o->frame_mtu = umtu + BTAP_ETHERNET_HEADER_LENGTH;
    }
    
    // set connected
    
    ULONG upstatus = TRUE;
    if (!DeviceIoControl(o->device, TAP_IOCTL_SET_MEDIA_STATUS, &upstatus, sizeof(upstatus), &upstatus, sizeof(upstatus), &len, NULL)) {
        BLog(BLOG_ERROR, "DeviceIoControl(TAP_IOCTL_SET_MEDIA_STATUS) failed");
        goto fail2;
    }
    
    BLog(BLOG_INFO, "Device opened");
    
    // associate device with IOCP
    
    if (!CreateIoCompletionPort(o->device, BReactor_GetIOCPHandle(o->reactor), 0, 0)) {
        BLog(BLOG_ERROR, "CreateIoCompletionPort failed");
        goto fail2;
    }
    
    // init send olap
    BReactorIOCPOverlapped_Init(&o->send_olap, o->reactor, o, NULL);
    
    // init recv olap
    BReactorIOCPOverlapped_Init(&o->recv_olap, o->reactor, o, (BReactorIOCPOverlapped_handler)recv_olap_handler);
    
    free(device_name);
    free(device_component_id);
    
    goto success;
    
fail2:
    ASSERT_FORCE(CloseHandle(o->device))
fail1:
    free(device_name);
    free(device_component_id);
fail0:
    return 0;
    
    #endif
    
    #if defined(BADVPN_LINUX) || defined(BADVPN_FREEBSD)
    
    o->close_fd = (init_data.init_type != BTAP_INIT_FD);
    
    switch (init_data.init_type) {
        case BTAP_INIT_FD: {
            ASSERT(init_data.init.fd.fd >= 0)
            ASSERT(init_data.init.fd.mtu >= 0)
            ASSERT(init_data.dev_type != BTAP_DEV_TAP || init_data.init.fd.mtu >= BTAP_ETHERNET_HEADER_LENGTH)
            
            o->fd = init_data.init.fd.fd;
            o->frame_mtu = init_data.init.fd.mtu;
        } break;
        
        case BTAP_INIT_STRING: {
            char devname_real[IFNAMSIZ];
            
            #ifdef BADVPN_LINUX
            
            // open device
            
            if ((o->fd = open("/dev/net/tun", O_RDWR)) < 0) {
                BLog(BLOG_ERROR, "error opening device");
                goto fail0;
            }
            
            // configure device
            
            struct ifreq ifr;
            memset(&ifr, 0, sizeof(ifr));
            ifr.ifr_flags |= IFF_NO_PI;
            if (init_data.dev_type == BTAP_DEV_TUN) {
                ifr.ifr_flags |= IFF_TUN;
            } else {
                ifr.ifr_flags |= IFF_TAP;
            }
            if (init_data.init.string) {
                snprintf(ifr.ifr_name, IFNAMSIZ, "%s", init_data.init.string);
            }
            
            if (ioctl(o->fd, TUNSETIFF, (void *)&ifr) < 0) {
                BLog(BLOG_ERROR, "error configuring device");
                goto fail1;
            }
            
            strcpy(devname_real, ifr.ifr_name);
            
            #endif
            
            #ifdef BADVPN_FREEBSD
            
            if (init_data.dev_type == BTAP_DEV_TUN) {
                BLog(BLOG_ERROR, "TUN not supported on FreeBSD");
                goto fail0;
            }
            
            if (!init_data.init.string) {
                BLog(BLOG_ERROR, "no device specified");
                goto fail0;
            }
            
            // open device
            
            char devnode[10 + IFNAMSIZ];
            snprintf(devnode, sizeof(devnode), "/dev/%s", init_data.init.string);
            
            if ((o->fd = open(devnode, O_RDWR)) < 0) {
                BLog(BLOG_ERROR, "error opening device");
                goto fail0;
            }
            
            // get name
            
            struct ifreq ifr;
            memset(&ifr, 0, sizeof(ifr));
            if (ioctl(o->fd, TAPGIFNAME, (void *)&ifr) < 0) {
                BLog(BLOG_ERROR, "error configuring device");
                goto fail1;
            }
            
            strcpy(devname_real, ifr.ifr_name);
            
            #endif
            
            // get MTU
            
            // open dummy socket for ioctls
            int sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock < 0) {
                BLog(BLOG_ERROR, "socket failed");
                goto fail1;
            }
            
            memset(&ifr, 0, sizeof(ifr));
            strcpy(ifr.ifr_name, devname_real);
            
            if (ioctl(sock, SIOCGIFMTU, (void *)&ifr) < 0) {
                BLog(BLOG_ERROR, "error getting MTU");
                close(sock);
                goto fail1;
            }
            
            if (init_data.dev_type == BTAP_DEV_TUN) {
                o->frame_mtu = ifr.ifr_mtu;
            } else {
                o->frame_mtu = ifr.ifr_mtu + BTAP_ETHERNET_HEADER_LENGTH;
            }
            
            close(sock);
        } break;
        
        default: ASSERT(0);
    }
        
    // set non-blocking
    if (fcntl(o->fd, F_SETFL, O_NONBLOCK) < 0) {
        BLog(BLOG_ERROR, "cannot set non-blocking");
        goto fail1;
    }
    
    // init file descriptor object
    BFileDescriptor_Init(&o->bfd, o->fd, (BFileDescriptor_handler)fd_handler, o);
    if (!BReactor_AddFileDescriptor(o->reactor, &o->bfd)) {
        BLog(BLOG_ERROR, "BReactor_AddFileDescriptor failed");
        goto fail1;
    }
    o->poll_events = 0;
    
    goto success;
    
fail1:
    if (o->close_fd) {
        ASSERT_FORCE(close(o->fd) == 0)
    }
fail0:
    return 0;
    
    #endif
    
success:
    // init output
    PacketRecvInterface_Init(&o->output, o->frame_mtu, (PacketRecvInterface_handler_recv)output_handler_recv, o, BReactor_PendingGroup(o->reactor));
    
    // set no output packet
    o->output_packet = NULL;
    
    DebugError_Init(&o->d_err, BReactor_PendingGroup(o->reactor));
    DebugObject_Init(&o->d_obj);
    return 1;
}

void BTap_Free (BTap *o)
{
    DebugObject_Free(&o->d_obj);
    DebugError_Free(&o->d_err);
    
    // free output
    PacketRecvInterface_Free(&o->output);
    
#ifdef BADVPN_USE_WINAPI
    
    // cancel I/O
    ASSERT_FORCE(CancelIo(o->device))
    
    // wait receiving to finish
    if (o->output_packet) {
        BLog(BLOG_DEBUG, "waiting for receiving to finish");
        BReactorIOCPOverlapped_Wait(&o->recv_olap, NULL, NULL);
    }
    
    // free recv olap
    BReactorIOCPOverlapped_Free(&o->recv_olap);
    
    // free send olap
    BReactorIOCPOverlapped_Free(&o->send_olap);
    
    // close device
    ASSERT_FORCE(CloseHandle(o->device))
    
#else
    
    // free BFileDescriptor
    BReactor_RemoveFileDescriptor(o->reactor, &o->bfd);
    
    if (o->close_fd) {
        // close file descriptor
        ASSERT_FORCE(close(o->fd) == 0)
    }
    
#endif
}

int BTap_GetMTU (BTap *o)
{
    DebugObject_Access(&o->d_obj);
    
    return o->frame_mtu;
}

void BTap_Send (BTap *o, uint8_t *data, int data_len)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    ASSERT(data_len >= 0)
    ASSERT(data_len <= o->frame_mtu)
    
#ifdef BADVPN_USE_WINAPI
    
    // ignore frames without an Ethernet header, or we get errors in WriteFile
    if (data_len < 14) {
        return;
    }
    
    memset(&o->send_olap.olap, 0, sizeof(o->send_olap.olap));
    
    // write
    BOOL res = WriteFile(o->device, data, data_len, NULL, &o->send_olap.olap);
    if (res == FALSE && GetLastError() != ERROR_IO_PENDING) {
        BLog(BLOG_ERROR, "WriteFile failed (%u)", GetLastError());
        return;
    }
    
    // wait
    int succeeded;
    DWORD bytes;
    BReactorIOCPOverlapped_Wait(&o->send_olap, &succeeded, &bytes);
    
    if (!succeeded) {
        BLog(BLOG_ERROR, "write operation failed");
    } else {
        ASSERT(bytes >= 0)
        ASSERT(bytes <= data_len)
        
        if (bytes < data_len) {
            BLog(BLOG_ERROR, "write operation didn't write everything");
        }
    }
    
#else
    
    int bytes = write(o->fd, data, data_len);
    if (bytes < 0) {
        // malformed packets will cause errors, ignore them and act like
        // the packet was accepeted
    } else {
        if (bytes != data_len) {
            BLog(BLOG_WARNING, "written %d expected %d", bytes, data_len);
        }
    }
    
#endif
}

PacketRecvInterface * BTap_GetOutput (BTap *o)
{
    DebugObject_Access(&o->d_obj);
    
    return &o->output;
}
