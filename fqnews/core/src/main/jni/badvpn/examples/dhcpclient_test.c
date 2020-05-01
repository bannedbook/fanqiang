/**
 * @file dhcpclient_test.c
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

#include <stddef.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <base/BLog.h>
#include <system/BReactor.h>
#include <system/BSignal.h>
#include <system/BTime.h>
#include <system/BNetwork.h>
#include <dhcpclient/BDHCPClient.h>

BReactor reactor;
BRandom2 random2;
BDHCPClient dhcp;

static void signal_handler (void *user);
static void dhcp_handler (void *unused, int event);

int main (int argc, char **argv)
{
    if (argc <= 0) {
        return 1;
    }
    
    if (argc != 2) {
        printf("Usage: %s <interface>\n", argv[0]);
        goto fail0;
    }
    
    char *ifname = argv[1];
    
    BTime_Init();
    
    BLog_InitStdout();
    
    if (!BNetwork_GlobalInit()) {
        DEBUG("BNetwork_GlobalInit failed");
        goto fail1;
    }
    
    if (!BReactor_Init(&reactor)) {
        DEBUG("BReactor_Init failed");
        goto fail1;
    }
    
    if (!BRandom2_Init(&random2, 0)) {
        DEBUG("BRandom2_Init failed");
        goto fail1a;
    }
    
    if (!BSignal_Init(&reactor, signal_handler, NULL)) {
        DEBUG("BSignal_Init failed");
        goto fail2;
    }
    
    struct BDHCPClient_opts opts = {};
    
    if (!BDHCPClient_Init(&dhcp, ifname, opts, &reactor, &random2, dhcp_handler, NULL)) {
        DEBUG("BDHCPClient_Init failed");
        goto fail3;
    }
    
    BReactor_Exec(&reactor);
    
    BDHCPClient_Free(&dhcp);
fail3:
    BSignal_Finish();
fail2:
    BRandom2_Free(&random2);
fail1a:
    BReactor_Free(&reactor);
fail1:
    BLog_Free();
fail0:
    DebugObjectGlobal_Finish();
    
    return 1;
}

void signal_handler (void *user)
{
    DEBUG("termination requested");
    
    BReactor_Quit(&reactor, 0);
}

void dhcp_handler (void *unused, int event)
{
    switch (event) {
        case BDHCPCLIENT_EVENT_UP: {
            printf("DHCP: up");
            
            uint32_t ip;
            uint8_t *ipb = (void *)&ip;
            
            BDHCPClient_GetClientIP(&dhcp, &ip);
            printf(" IP=%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8, ipb[0], ipb[1], ipb[2], ipb[3]);
            
            BDHCPClient_GetClientMask(&dhcp, &ip);
            printf(" Mask=%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8, ipb[0], ipb[1], ipb[2], ipb[3]);
            
            if (BDHCPClient_GetRouter(&dhcp, &ip)) {
                printf(" Router=%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8, ipb[0], ipb[1], ipb[2], ipb[3]);
            }
            
            uint32_t dns[BDHCPCLIENT_MAX_DOMAIN_NAME_SERVERS];
            int num = BDHCPClient_GetDNS(&dhcp, dns, BDHCPCLIENT_MAX_DOMAIN_NAME_SERVERS);
            for (int i = 0; i < num; i++) {
                ip=dns[i];
                printf(" DNS=%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8, ipb[0], ipb[1], ipb[2], ipb[3]);
            }
            
            printf("\n");
        } break;
        
        case BDHCPCLIENT_EVENT_DOWN: {
            printf("DHCP: down\n");
        } break;
        
        case BDHCPCLIENT_EVENT_ERROR: {
            printf("DHCP: error\n");
            
            // exit reactor
            BReactor_Quit(&reactor, 0);
        } break;
        
        default:
            ASSERT(0);
    }
}
