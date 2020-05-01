/**
 * @file NCDVal_types.h
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

#ifndef BADVPN_NCDVAL_TYPES_H
#define BADVPN_NCDVAL_TYPES_H

#include <stddef.h>

#include <misc/maxalign.h>

#define NCDVAL_FASTBUF_SIZE 64
#define NCDVAL_MAXIDX INT_MAX
#define NCDVAL_MINIDX INT_MIN
#define NCDVAL_TOPPLID (-1 - NCDVAL_MINIDX)

typedef int NCDVal__idx;

typedef struct {
    NCDStringIndex *string_index;
    NCDVal__idx size;
    NCDVal__idx used;
    NCDVal__idx first_ref;
    union {
        char fastbuf[NCDVAL_FASTBUF_SIZE];
        char *allocd_buf;
        bmax_align_t align_max;
    };
} NCDValMem;

typedef struct {
    NCDValMem *mem;
    NCDVal__idx idx;
} NCDValRef;

typedef struct {
    NCDVal__idx idx;
} NCDValSafeRef;

typedef struct {
    NCDVal__idx elemidx;
} NCDValMapElem;

struct NCDVal__instr;

typedef struct {
    struct NCDVal__instr *instrs;
    size_t num_instrs;
} NCDValReplaceProg;

typedef struct {
    char *data;
    int is_allocated;
} NCDValNullTermString;

#endif
