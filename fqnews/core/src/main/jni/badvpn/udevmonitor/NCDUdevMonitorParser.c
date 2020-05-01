/**
 * @file NCDUdevMonitorParser.c
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

#include <misc/string_begins_with.h>
#include <misc/balloc.h>
#include <base/BLog.h>

#include <udevmonitor/NCDUdevMonitorParser.h>

#include <generated/blog_channel_NCDUdevMonitorParser.h>

#define PROPERTY_REGEX "^([^=]+)=(.*)$"

static uint8_t * find_end (uint8_t *buf, size_t len)
{
    while (len >= 2) {
        if (buf[0] == '\n' && buf[1] == '\n') {
            return (buf + 2);
        }
        buf++;
        len--;
    }
    
    return NULL;
}

static int parse_property (NCDUdevMonitorParser *o, char *data)
{
    ASSERT(o->ready_num_properties >= 0)
    ASSERT(o->ready_num_properties <= o->max_properties)
    
    if (o->ready_num_properties == o->max_properties) {
        BLog(BLOG_ERROR, "too many properties");
        return 0;
    }
    struct NCDUdevMonitorParser_property *prop = &o->ready_properties[o->ready_num_properties];
    
    // execute property regex
    regmatch_t matches[3];
    if (regexec(&o->property_regex, data, 3, matches, 0) != 0) {
        BLog(BLOG_ERROR, "failed to parse property");
        return 0;
    }
    
    // extract components
    prop->name = data + matches[1].rm_so;
    *(data + matches[1].rm_eo) = '\0';
    prop->value = data + matches[2].rm_so;
    *(data + matches[2].rm_eo) = '\0';
    
    // register property
    o->ready_num_properties++;
    
    return 1;
}

static int parse_message (NCDUdevMonitorParser *o)
{
    ASSERT(!o->is_ready)
    ASSERT(o->ready_len >= 2)
    ASSERT(o->buf[o->ready_len - 2] == '\n')
    ASSERT(o->buf[o->ready_len - 1] == '\n')
    
    // zero terminate message (replacing the second newline)
    o->buf[o->ready_len - 1] = '\0';
    
    // start parsing
    char *line = (char *)o->buf;
    int first_line = 1;
    
    // set is not ready event
    o->ready_is_ready_event = 0;
    
    // init properties
    o->ready_num_properties = 0;
    
    do {
        // find end of line
        char *line_end = strchr(line, '\n');
        ASSERT(line_end)
        
        // zero terminate line
        *line_end = '\0';
        
        if (o->is_info_mode) {
            // ignore W: entries with missing space
            if (string_begins_with(line, "W:")) {
                goto nextline;
            }
            
            // parse prefix
            if (strlen(line) < 3 || line[1] != ':' || line[2] != ' ') {
                BLog(BLOG_ERROR, "failed to parse head");
                return 0;
            }
            char line_type = line[0];
            char *line_value = line + 3;
            
            if (first_line) {
                if (line_type != 'P') {
                    BLog(BLOG_ERROR, "wrong first line type");
                    return 0;
                }
            } else {
                if (line_type == 'E') {
                    if (!parse_property(o, line_value)) {
                        return 0;
                    }
                }
            }
        } else {
            if (first_line) {
                // is this the initial informational message?
                if (string_begins_with(line, "monitor")) {
                    o->ready_is_ready_event = 1;
                    break;
                }
                
                // check first line
                if (!string_begins_with(line, "UDEV  ") && !string_begins_with(line, "KERNEL")) {
                    BLog(BLOG_ERROR, "failed to parse head");
                    return 0;
                }
            } else {
                if (!parse_property(o, line)) {
                    return 0;
                }
            }
        }
    nextline:
        
        first_line = 0;
        line = line_end + 1;
    } while (*line);
    
    // set ready
    o->is_ready = 1;
    
    return 1;
}

static void process_data (NCDUdevMonitorParser *o)
{
    ASSERT(!o->is_ready)
    
    while (1) {
        // look for end of event
        uint8_t *c = find_end(o->buf, o->buf_used);
        if (!c) {
            // check for out of buffer condition
            if (o->buf_used == o->buf_size) {
                BLog(BLOG_ERROR, "out of buffer");
                o->buf_used = 0;
            }
            
            // receive more data
            StreamRecvInterface_Receiver_Recv(o->input, o->buf + o->buf_used, o->buf_size - o->buf_used);
            return;
        }
        
        // remember message length
        o->ready_len = c - o->buf;
        
        // parse message
        if (parse_message(o)) {
            break;
        }
        
        // shift buffer
        memmove(o->buf, o->buf + o->ready_len, o->buf_used - o->ready_len);
        o->buf_used -= o->ready_len;
    }
    
    // call handler
    o->handler(o->user);
    return;
}

static void input_handler_done (NCDUdevMonitorParser *o, int data_len)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(!o->is_ready)
    ASSERT(data_len > 0)
    ASSERT(data_len <= o->buf_size - o->buf_used)
    
    // increment buffer position
    o->buf_used += data_len;
    
    // process data
    process_data(o);
    return;
}

static void done_job_handler (NCDUdevMonitorParser *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->is_ready)
    
    // shift buffer
    memmove(o->buf, o->buf + o->ready_len, o->buf_used - o->ready_len);
    o->buf_used -= o->ready_len;
    
    // set not ready
    o->is_ready = 0;
    
    // process data
    process_data(o);
    return;
}

int NCDUdevMonitorParser_Init (NCDUdevMonitorParser *o, StreamRecvInterface *input, int buf_size, int max_properties,
                               int is_info_mode, BPendingGroup *pg, void *user,
                               NCDUdevMonitorParser_handler handler)
{
    ASSERT(buf_size > 0)
    ASSERT(max_properties >= 0)
    ASSERT(is_info_mode == 0 || is_info_mode == 1)
    
    // init arguments
    o->input = input;
    o->buf_size = buf_size;
    o->max_properties = max_properties;
    o->is_info_mode = is_info_mode;
    o->user = user;
    o->handler = handler;
    
    // init input
    StreamRecvInterface_Receiver_Init(o->input, (StreamRecvInterface_handler_done)input_handler_done, o);
    
    // init property regex
    if (regcomp(&o->property_regex, PROPERTY_REGEX, REG_EXTENDED) != 0) {
        BLog(BLOG_ERROR, "regcomp failed");
        goto fail1;
    }
    
    // init done job
    BPending_Init(&o->done_job, pg, (BPending_handler)done_job_handler, o);
    
    // allocate buffer
    if (!(o->buf = malloc(o->buf_size))) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail2;
    }
    
    // set buffer position
    o->buf_used = 0;
    
    // set not ready
    o->is_ready = 0;
    
    // allocate properties
    if (!(o->ready_properties = BAllocArray(o->max_properties, sizeof(o->ready_properties[0])))) {
        BLog(BLOG_ERROR, "BAllocArray failed");
        goto fail3;
    }
    
    // start receiving
    StreamRecvInterface_Receiver_Recv(o->input, o->buf, o->buf_size);
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail3:
    free(o->buf);
fail2:
    BPending_Free(&o->done_job);
    regfree(&o->property_regex);
fail1:
    return 0;
}

void NCDUdevMonitorParser_Free (NCDUdevMonitorParser *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free properties
    BFree(o->ready_properties);
    
    // free buffer
    free(o->buf);
    
    // free done job
    BPending_Free(&o->done_job);
    
    // free property regex
    regfree(&o->property_regex);
}

void NCDUdevMonitorParser_AssertReady (NCDUdevMonitorParser *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->is_ready)
}

void NCDUdevMonitorParser_Done (NCDUdevMonitorParser *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->is_ready)
    
    // schedule done job
    BPending_Set(&o->done_job);
}

int NCDUdevMonitorParser_IsReadyEvent (NCDUdevMonitorParser *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->is_ready)
    
    return o->ready_is_ready_event;
}

int NCDUdevMonitorParser_GetNumProperties (NCDUdevMonitorParser *o)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->is_ready)
    
    return o->ready_num_properties;
}

void NCDUdevMonitorParser_GetProperty (NCDUdevMonitorParser *o, int index, const char **name, const char **value)
{
    DebugObject_Access(&o->d_obj);
    ASSERT(o->is_ready)
    ASSERT(index >= 0)
    ASSERT(index < o->ready_num_properties)
    
    *name = o->ready_properties[index].name;
    *value = o->ready_properties[index].value;
}
