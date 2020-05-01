/**
 * @file NCDUdevMonitorParser.h
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

#ifndef BADVPN_UDEVMONITOR_NCDUDEVMONITORPARSER_H
#define BADVPN_UDEVMONITOR_NCDUDEVMONITORPARSER_H

#include <stdint.h>
#include <regex.h>

#include <misc/debug.h>
#include <base/DebugObject.h>
#include <flow/StreamRecvInterface.h>

typedef void (*NCDUdevMonitorParser_handler) (void *user);

struct NCDUdevMonitorParser_property {
    char *name;
    char *value;
};

typedef struct {
    StreamRecvInterface *input;
    int buf_size;
    int max_properties;
    int is_info_mode;
    void *user;
    NCDUdevMonitorParser_handler handler;
    regex_t property_regex;
    BPending done_job;
    uint8_t *buf;
    int buf_used;
    int is_ready;
    int ready_len;
    int ready_is_ready_event;
    struct NCDUdevMonitorParser_property *ready_properties;
    int ready_num_properties;
    DebugObject d_obj;
} NCDUdevMonitorParser;

int NCDUdevMonitorParser_Init (NCDUdevMonitorParser *o, StreamRecvInterface *input, int buf_size, int max_properties,
                               int is_info_mode, BPendingGroup *pg, void *user,
                               NCDUdevMonitorParser_handler handler) WARN_UNUSED;
void NCDUdevMonitorParser_Free (NCDUdevMonitorParser *o);
void NCDUdevMonitorParser_AssertReady (NCDUdevMonitorParser *o);
void NCDUdevMonitorParser_Done (NCDUdevMonitorParser *o);
int NCDUdevMonitorParser_IsReadyEvent (NCDUdevMonitorParser *o);
int NCDUdevMonitorParser_GetNumProperties (NCDUdevMonitorParser *o);
void NCDUdevMonitorParser_GetProperty (NCDUdevMonitorParser *o, int index, const char **name, const char **value);

#endif
