/**
 * @file ncdudevmanager_test.c
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <misc/debug.h>
#include <system/BTime.h>
#include <base/BLog.h>
#include <system/BReactor.h>
#include <system/BUnixSignal.h>
#include <system/BProcess.h>
#include <system/BNetwork.h>
#include <udevmonitor/NCDUdevManager.h>

BReactor reactor;
BUnixSignal usignal;
BProcessManager manager;
NCDUdevManager umanager;
NCDUdevClient client;

static void signal_handler (void *user, int signo);
static void client_handler (void *unused, char *devpath, int have_map, BStringMap map);

int main (int argc, char **argv)
{
    if (!(argc == 1 || (argc == 2 && !strcmp(argv[1], "--no-udev")))) {
        fprintf(stderr, "Usage: %s [--no-udev]\n", (argc > 0 ? argv[0] : NULL));
        goto fail0;
    }
    
    int no_udev = (argc == 2);
    
    if (!BNetwork_GlobalInit()) {
        DEBUG("BNetwork_GlobalInit failed");
        goto fail0;
    }
    
    BTime_Init();
    
    BLog_InitStdout();
    
    if (!BReactor_Init(&reactor)) {
        DEBUG("BReactor_Init failed");
        goto fail1;
    }
    
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGHUP);
    if (!BUnixSignal_Init(&usignal, &reactor, set, signal_handler, NULL)) {
        fprintf(stderr, "BUnixSignal_Init failed\n");
        goto fail2;
    }
    
    if (!BProcessManager_Init(&manager, &reactor)) {
        DEBUG("BProcessManager_Init failed");
        goto fail3;
    }
    
    NCDUdevManager_Init(&umanager, no_udev, &reactor, &manager);
    
    NCDUdevClient_Init(&client, &umanager, NULL, client_handler);
    
    BReactor_Exec(&reactor);
    
    NCDUdevClient_Free(&client);
    
    NCDUdevManager_Free(&umanager);
    
    BProcessManager_Free(&manager);
fail3:
    BUnixSignal_Free(&usignal, 0);
fail2:
    BReactor_Free(&reactor);
fail1:
    BLog_Free();
fail0:
    DebugObjectGlobal_Finish();
    
    return 1;
}

static void signal_handler (void *user, int signo)
{
    if (signo == SIGHUP) {
        fprintf(stderr, "received SIGHUP, restarting client\n");
        
        NCDUdevClient_Free(&client);
        NCDUdevClient_Init(&client, &umanager, NULL, client_handler);
    } else {
        fprintf(stderr, "received %s, exiting\n", (signo == SIGINT ? "SIGINT" : "SIGTERM"));
        
        // exit event loop
        BReactor_Quit(&reactor, 1);
    }
}

void client_handler (void *unused, char *devpath, int have_map, BStringMap map)
{
    printf("event %s\n", devpath);
    
    if (!have_map) {
        printf("  no map\n");
    } else {
        printf("  map:\n");
        
        const char *name = BStringMap_First(&map);
        while (name) {
            printf("    %s=%s\n", name, BStringMap_Get(&map, name));
            name = BStringMap_Next(&map, name);
        }
    }
    
    const BStringMap *cache_map = NCDUdevManager_Query(&umanager, devpath);
    if (!cache_map) {
        printf("  no cache\n");
    } else {
        printf("  cache:\n");
        
        const char *name = BStringMap_First(cache_map);
        while (name) {
            printf("    %s=%s\n", name, BStringMap_Get(cache_map, name));
            name = BStringMap_Next(cache_map, name);
        }
    }
    
    if (have_map) {
        BStringMap_Free(&map);
    }
    free(devpath);
}
