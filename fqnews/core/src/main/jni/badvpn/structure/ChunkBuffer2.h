/**
 * @file ChunkBuffer2.h
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
 * Circular packet buffer
 */

#ifndef BADVPN_STRUCTURE_CHUNKBUFFER2_H
#define BADVPN_STRUCTURE_CHUNKBUFFER2_H

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include <misc/balign.h>
#include <misc/debug.h>

#ifndef NDEBUG
#define CHUNKBUFFER2_ASSERT_BUFFER(_buf) _ChunkBuffer2_assert_buffer(_buf);
#define CHUNKBUFFER2_ASSERT_IO(_buf) _ChunkBuffer2_assert_io(_buf);
#else
#define CHUNKBUFFER2_ASSERT_BUFFER(_buf)
#define CHUNKBUFFER2_ASSERT_IO(_buf)
#endif

struct ChunkBuffer2_block {
    int len;
};

typedef struct {
    struct ChunkBuffer2_block *buffer;
    int size;
    int wrap;
    int start;
    int used;
    int mtu;
    uint8_t *input_dest;
    int input_avail;
    uint8_t *output_dest;
    int output_avail;
} ChunkBuffer2;

// calculates a buffer size needed to hold at least 'num' packets long at least 'chunk_len'
static int ChunkBuffer2_calc_blocks (int chunk_len, int num);

// initialize
static void ChunkBuffer2_Init (ChunkBuffer2 *buf, struct ChunkBuffer2_block *buffer, int blocks, int mtu);

// submit a packet written to the buffer
static void ChunkBuffer2_SubmitPacket (ChunkBuffer2 *buf, int len);

// remove the first packet
static void ChunkBuffer2_ConsumePacket (ChunkBuffer2 *buf);

static int _ChunkBuffer2_end (ChunkBuffer2 *buf)
{
    if (buf->used >= buf->wrap - buf->start) {
        return (buf->used - (buf->wrap - buf->start));
    } else {
        return (buf->start + buf->used);
    }
}

#ifndef NDEBUG

static void _ChunkBuffer2_assert_buffer (ChunkBuffer2 *buf)
{
    ASSERT(buf->size > 0)
    ASSERT(buf->wrap > 0)
    ASSERT(buf->wrap <= buf->size)
    ASSERT(buf->start >= 0)
    ASSERT(buf->start < buf->wrap)
    ASSERT(buf->used >= 0)
    ASSERT(buf->used <= buf->wrap)
    ASSERT(buf->wrap == buf->size || buf->used >= buf->wrap - buf->start)
    ASSERT(buf->mtu >= 0)
}

static void _ChunkBuffer2_assert_io (ChunkBuffer2 *buf)
{
    // check input
    
    int end = _ChunkBuffer2_end(buf);
    
    if (buf->size - end - 1 < buf->mtu) {
        // it will never be possible to write a MTU long packet here
        ASSERT(!buf->input_dest)
        ASSERT(buf->input_avail == -1)
    } else {
        // calculate number of free blocks
        int free;
        if (buf->used >= buf->wrap - buf->start) {
            free = buf->start - end;
        } else {
            free = buf->size - end;
        }
        
        if (free > 0) {
            // got space at least for a header. More space will become available as packets are
            // read from the buffer, up to MTU.
            ASSERT(buf->input_dest == (uint8_t *)&buf->buffer[end + 1])
            ASSERT(buf->input_avail == (free - 1) * sizeof(struct ChunkBuffer2_block))
        } else {
            // no space
            ASSERT(!buf->input_dest)
            ASSERT(buf->input_avail == -1)
        }
    }
    
    // check output
    
    if (buf->used > 0) {
        int datalen = buf->buffer[buf->start].len;
        ASSERT(datalen >= 0)
        int blocklen = bdivide_up(datalen, sizeof(struct ChunkBuffer2_block));
        ASSERT(blocklen <= buf->used - 1)
        ASSERT(blocklen <= buf->wrap - buf->start - 1)
        ASSERT(buf->output_dest == (uint8_t *)&buf->buffer[buf->start + 1])
        ASSERT(buf->output_avail == datalen)
    } else {
        ASSERT(!buf->output_dest)
        ASSERT(buf->output_avail == -1)
    }
}

#endif

static void _ChunkBuffer2_update_input (ChunkBuffer2 *buf)
{
    int end = _ChunkBuffer2_end(buf);
    
    if (buf->size - end - 1 < buf->mtu) {
        // it will never be possible to write a MTU long packet here
        buf->input_dest = NULL;
        buf->input_avail = -1;
        return;
    }
    
    // calculate number of free blocks
    int free;
    if (buf->used >= buf->wrap - buf->start) {
        free = buf->start - end;
    } else {
        free = buf->size - end;
    }
    
    if (free > 0) {
        // got space at least for a header. More space will become available as packets are
        // read from the buffer, up to MTU.
        buf->input_dest = (uint8_t *)&buf->buffer[end + 1];
        buf->input_avail = (free - 1) * sizeof(struct ChunkBuffer2_block);
    } else {
        // no space
        buf->input_dest = NULL;
        buf->input_avail = -1;
    }
}

static void _ChunkBuffer2_update_output (ChunkBuffer2 *buf)
{
    if (buf->used > 0) {
        int datalen = buf->buffer[buf->start].len;
        ASSERT(datalen >= 0)
#ifndef NDEBUG
        int blocklen = bdivide_up(datalen, sizeof(struct ChunkBuffer2_block));
        ASSERT(blocklen <= buf->used - 1)
        ASSERT(blocklen <= buf->wrap - buf->start - 1)
#endif
        buf->output_dest = (uint8_t *)&buf->buffer[buf->start + 1];
        buf->output_avail = datalen;
    } else {
        buf->output_dest = NULL;
        buf->output_avail = -1;
    }
}

int ChunkBuffer2_calc_blocks (int chunk_len, int num)
{
    int chunk_data_blocks = bdivide_up(chunk_len, sizeof(struct ChunkBuffer2_block));
    
    if (chunk_data_blocks > INT_MAX - 1) {
        return -1;
    }
    int chunk_blocks = 1 + chunk_data_blocks;
    
    if (num > INT_MAX - 1) {
        return -1;
    }
    int num_chunks = num + 1;
    
    if (chunk_blocks > INT_MAX / num_chunks) {
        return -1;
    }
    int blocks = chunk_blocks * num_chunks;
    
    return blocks;
}

void ChunkBuffer2_Init (ChunkBuffer2 *buf, struct ChunkBuffer2_block *buffer, int blocks, int mtu)
{
    ASSERT(blocks > 0)
    ASSERT(mtu >= 0)
    
    buf->buffer = buffer;
    buf->size = blocks;
    buf->wrap = blocks;
    buf->start = 0;
    buf->used = 0;
    buf->mtu = bdivide_up(mtu, sizeof(struct ChunkBuffer2_block));
    
    CHUNKBUFFER2_ASSERT_BUFFER(buf)
    
    _ChunkBuffer2_update_input(buf);
    _ChunkBuffer2_update_output(buf);
    
    CHUNKBUFFER2_ASSERT_IO(buf)
}

void ChunkBuffer2_SubmitPacket (ChunkBuffer2 *buf, int len)
{
    ASSERT(buf->input_dest)
    ASSERT(len >= 0)
    ASSERT(len <= buf->input_avail)
    
    CHUNKBUFFER2_ASSERT_BUFFER(buf)
    CHUNKBUFFER2_ASSERT_IO(buf)
    
    int end = _ChunkBuffer2_end(buf);
    int blocklen = bdivide_up(len, sizeof(struct ChunkBuffer2_block));
    
    ASSERT(blocklen <= buf->size - end - 1)
    ASSERT(buf->used < buf->wrap - buf->start || blocklen <= buf->start - end - 1)
    
    buf->buffer[end].len = len;
    buf->used += 1 + blocklen;
    
    if (buf->used <= buf->wrap - buf->start && buf->mtu > buf->size - (end + 1 + blocklen) - 1) {
        buf->wrap = end + 1 + blocklen;
    }
    
    CHUNKBUFFER2_ASSERT_BUFFER(buf)
    
    // update input
    _ChunkBuffer2_update_input(buf);
    
    // update output
    if (buf->used == 1 + blocklen) {
        _ChunkBuffer2_update_output(buf);
    }
    
    CHUNKBUFFER2_ASSERT_IO(buf)
}

void ChunkBuffer2_ConsumePacket (ChunkBuffer2 *buf)
{
    ASSERT(buf->output_dest)
    
    CHUNKBUFFER2_ASSERT_BUFFER(buf)
    CHUNKBUFFER2_ASSERT_IO(buf)
    
    ASSERT(1 <= buf->wrap - buf->start)
    ASSERT(1 <= buf->used)
    
    int blocklen = bdivide_up(buf->buffer[buf->start].len, sizeof(struct ChunkBuffer2_block));
    
    ASSERT(blocklen <= buf->wrap - buf->start - 1)
    ASSERT(blocklen <= buf->used - 1)
    
    int data_wrapped = (buf->used >= buf->wrap - buf->start);
    
    buf->start += 1 + blocklen;
    buf->used -= 1 + blocklen;
    if (buf->start == buf->wrap) {
        buf->start = 0;
        buf->wrap = buf->size;
    }
    
    CHUNKBUFFER2_ASSERT_BUFFER(buf)
    
    // update input
    if (data_wrapped) {
        _ChunkBuffer2_update_input(buf);
    }
    
    // update output
    _ChunkBuffer2_update_output(buf);
    
    CHUNKBUFFER2_ASSERT_IO(buf)
}

#endif
