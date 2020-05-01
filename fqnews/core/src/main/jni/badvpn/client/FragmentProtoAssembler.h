/**
 * @file FragmentProtoAssembler.h
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
 * Object which decodes packets according to FragmentProto.
 */

#ifndef BADVPN_CLIENT_FRAGMENTPROTOASSEMBLER_H
#define BADVPN_CLIENT_FRAGMENTPROTOASSEMBLER_H

#include <stdint.h>

#include <protocol/fragmentproto.h>
#include <misc/debug.h>
#include <misc/compare.h>
#include <base/DebugObject.h>
#include <base/BLog.h>
#include <structure/LinkedList1.h>
#include <structure/SAvl.h>
#include <flow/PacketPassInterface.h>

#define FPA_MAX_TIME UINT32_MAX

struct FragmentProtoAssembler_frame;

#include "FragmentProtoAssembler_tree.h"
#include <structure/SAvl_decl.h>

struct FragmentProtoAssembler_chunk {
    int start;
    int len;
};

struct FragmentProtoAssembler_frame {
    LinkedList1Node list_node; // node in free or used list
    struct FragmentProtoAssembler_chunk *chunks; // array of chunks, up to num_chunks
    uint8_t *buffer; // buffer with frame data, size output_mtu
    // everything below only defined when frame entry is used
    fragmentproto_frameid id; // frame identifier
    uint32_t time; // packet time when the last chunk was received
    FPAFramesTreeNode tree_node; // node fields in tree for searching frames by id
    int num_chunks; // number of valid chunks
    int sum; // sum of all chunks' lengths
    int length; // length of the frame, or -1 if not yet known
    int length_so_far; // if length=-1, current data set's upper bound
};

/**
 * Object which decodes packets according to FragmentProto.
 *
 * Input is with {@link PacketPassInterface}.
 * Output is with {@link PacketPassInterface}.
 */
typedef struct {
    void *user;
    BLog_logfunc logfunc;
    PacketPassInterface input;
    PacketPassInterface *output;
    int output_mtu;
    int num_chunks;
    uint32_t time;
    int time_tolerance;
    struct FragmentProtoAssembler_frame *frames_entries;
    struct FragmentProtoAssembler_chunk *frames_chunks;
    uint8_t *frames_buffer;
    LinkedList1 frames_free;
    LinkedList1 frames_used;
    FPAFramesTree frames_used_tree;
    int in_len;
    uint8_t *in;
    int in_pos;
    DebugObject d_obj;
} FragmentProtoAssembler;

/**
 * Initializes the object.
 * {@link BLog_Init} must have been done.
 *
 * @param o the object
 * @param input_mtu maximum input packet size. Must be >=0.
 * @param output output interface
 * @param num_frames number of frames we can hold. Must be >0 and < FPA_MAX_TIME.
 *  To make the assembler tolerate out-of-order input of degree D, set to D+2.
 *  Here, D is the minimum size of a hypothetical buffer needed to order the input.
 * @param num_chunks maximum number of chunks a frame can come in. Must be >0.
 * @param pg pending group
 * @param user argument to handlers
 * @param logfunc function which prepends the log prefix using {@link BLog_Append}
 * @return 1 on success, 0 on failure
 */
int FragmentProtoAssembler_Init (FragmentProtoAssembler *o, int input_mtu, PacketPassInterface *output, int num_frames, int num_chunks, BPendingGroup *pg, void *user, BLog_logfunc logfunc) WARN_UNUSED;

/**
 * Frees the object.
 *
 * @param o the object
 */
void FragmentProtoAssembler_Free (FragmentProtoAssembler *o);

/**
 * Returns the input interface.
 *
 * @param o the object
 * @return input interface
 */
PacketPassInterface * FragmentProtoAssembler_GetInput (FragmentProtoAssembler *o);

#endif
