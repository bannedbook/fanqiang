/**
 * @file ncd-request.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <misc/string_begins_with.h>
#include <base/BLog.h>
#include <base/DebugObject.h>
#include <system/BNetwork.h>
#include <system/BReactor.h>
#include <system/BAddr.h>
#include <ncd/NCDValParser.h>
#include <ncd/NCDValGenerator.h>
#include <ncd/extra/NCDRequestClient.h>

#include <generated/blog_channel_ncd_request.h>

static void client_handler_error (void *user);
static void client_handler_connected (void *user);
static void request_handler_sent (void *user);
static void request_handler_reply (void *user, NCDValMem reply_mem, NCDValRef reply_value);
static void request_handler_finished (void *user, int is_error);
static int write_all (int fd, const uint8_t *data, size_t len);
static int make_connect_addr (const char *str, struct BConnection_addr *out_addr);

NCDStringIndex string_index;
NCDValMem request_mem;
NCDValRef request_value;
BReactor reactor;
NCDRequestClient client;
NCDRequestClientRequest request;
int have_request;

int main (int argc, char *argv[])
{
    int res = 1;
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s < unix:<socket_path> / tcp:<address>:<port> > <request_payload>\n", (argc > 0 ? argv[0] : ""));
        goto fail0;
    }
    
    char *connect_address = argv[1];
    char *request_payload_string = argv[2];
    
    BLog_InitStderr();
    
    BTime_Init();
    
    if (!NCDStringIndex_Init(&string_index)) {
        BLog(BLOG_ERROR, "NCDStringIndex_Init failed");
        goto fail01;
    }
    
    NCDValMem_Init(&request_mem, &string_index);
    
    if (!NCDValParser_Parse(MemRef_MakeCstr(request_payload_string), &request_mem, &request_value)) {
        BLog(BLOG_ERROR, "BReactor_Init failed");
        goto fail1;
    }
    
    if (!BNetwork_GlobalInit()) {
        BLog(BLOG_ERROR, "BNetwork_Init failed");
        goto fail1;
    }
    
    if (!BReactor_Init(&reactor)) {
        BLog(BLOG_ERROR, "BReactor_Init failed");
        goto fail1;
    }
    
    struct BConnection_addr addr;
    if (!make_connect_addr(connect_address, &addr)) {
        goto fail2;
    }
    
    if (!NCDRequestClient_Init(&client, addr, &reactor, &string_index, NULL, client_handler_error, client_handler_connected)) {
        BLog(BLOG_ERROR, "NCDRequestClient_Init failed");
        goto fail2;
    }
    
    have_request = 0;
    
    res = BReactor_Exec(&reactor);
    
    if (have_request) {
        NCDRequestClientRequest_Free(&request);
    }
    NCDRequestClient_Free(&client);
fail2:
    BReactor_Free(&reactor);
fail1:
    NCDValMem_Free(&request_mem);
    NCDStringIndex_Free(&string_index);
fail01:
    BLog_Free();
fail0:
    DebugObjectGlobal_Finish();
    return res;
}

static int make_connect_addr (const char *str, struct BConnection_addr *out_addr)
{
    size_t i;
    
    if (i = string_begins_with(str, "unix:")) {
        *out_addr = BConnection_addr_unix(MemRef_MakeCstr(str + i));
    }
    else if (i = string_begins_with(str, "tcp:")) {
        BAddr baddr;
        if (!BAddr_Parse2(&baddr, (char *)str + i, NULL, 0, 1)) {
            BLog(BLOG_ERROR, "failed to parse tcp address");
            return 0;
        }
        
        *out_addr = BConnection_addr_baddr(baddr);
    }
    else {
        BLog(BLOG_ERROR, "address must start with unix: or tcp:");
        return 0;
    }
    
    return 1;
}

static void client_handler_error (void *user)
{
    BLog(BLOG_ERROR, "client error");
    
    BReactor_Quit(&reactor, 1);
}

static void client_handler_connected (void *user)
{
    ASSERT(!have_request)
    
    if (!NCDRequestClientRequest_Init(&request, &client, request_value, NULL, request_handler_sent, request_handler_reply, request_handler_finished)) {
        BLog(BLOG_ERROR, "NCDRequestClientRequest_Init failed");
        BReactor_Quit(&reactor, 1);
        return;
    }
    
    have_request = 1;
}

static void request_handler_sent (void *user)
{
    ASSERT(have_request)
}

static void request_handler_reply (void *user, NCDValMem reply_mem, NCDValRef reply_value)
{
    ASSERT(have_request)
    
    char *str = NCDValGenerator_Generate(reply_value);
    if (!str) {
        BLog(BLOG_ERROR, "NCDValGenerator_Generate failed");
        goto fail0;
    }
    
    if (!write_all(1, (uint8_t *)str, strlen(str))) {
        goto fail1;
    }
    if (!write_all(1, (const uint8_t *)"\n", 1)) {
        goto fail1;
    }
    
    free(str);
    NCDValMem_Free(&reply_mem);
    return;
    
fail1:
    free(str);
fail0:
    NCDValMem_Free(&reply_mem);
    BReactor_Quit(&reactor, 1);
}

static void request_handler_finished (void *user, int is_error)
{
    if (is_error) {
        BLog(BLOG_ERROR, "request error");
        BReactor_Quit(&reactor, 1);
        return;
    }
    
    BReactor_Quit(&reactor, 0);
}

static int write_all (int fd, const uint8_t *data, size_t len)
{
    while (len > 0) {
        ssize_t res = write(fd, data, len);
        if (res <= 0) {
            BLog(BLOG_ERROR, "write failed");
            return 0;
        }
        data += res;
        len -= res;
    }
    
    return 1;
}
