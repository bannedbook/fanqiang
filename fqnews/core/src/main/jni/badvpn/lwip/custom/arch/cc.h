/**
 * @file cc.h
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

#ifndef LWIP_CUSTOM_CC_H
#define LWIP_CUSTOM_CC_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include <misc/debug.h>
#include <misc/byteorder.h>
#include <misc/packed.h>
#include <misc/print_macros.h>
#include <misc/byteorder.h>
#include <base/BLog.h>

#define PACK_STRUCT_BEGIN B_START_PACKED
#define PACK_STRUCT_END B_END_PACKED
#define PACK_STRUCT_STRUCT B_PACKED

#define LWIP_PLATFORM_DIAG(x) { if (BLog_WouldLog(BLOG_CHANNEL_lwip, BLOG_INFO)) { BLog_Begin(); BLog_Append x; BLog_Finish(BLOG_CHANNEL_lwip, BLOG_INFO); } }
#define LWIP_PLATFORM_ASSERT(x) { fprintf(stderr, "%s: lwip assertion failure: %s\n", __FUNCTION__, (x)); abort(); }

#define lwip_htons(x) hton16(x)
#define lwip_htonl(x) hton32(x)

#define LWIP_RAND() ( \
    (((uint32_t)(rand() & 0xFF)) << 24) | \
    (((uint32_t)(rand() & 0xFF)) << 16) | \
    (((uint32_t)(rand() & 0xFF)) << 8) | \
    (((uint32_t)(rand() & 0xFF)) << 0) \
)

// for BYTE_ORDER
#if defined(BADVPN_USE_WINAPI) && !defined(_MSC_VER)
    #include <sys/param.h>
#elif defined(BADVPN_LINUX)
    #include <endian.h>
#elif defined(BADVPN_FREEBSD)
    #include <machine/endian.h>
#else
    #define LITTLE_ENDIAN 1234
    #define BIG_ENDIAN 4321
    #if defined(BADVPN_LITTLE_ENDIAN)
        #define BYTE_ORDER LITTLE_ENDIAN
    #else
        #define BYTE_ORDER BIG_ENDIAN
    #endif
#endif

#endif
