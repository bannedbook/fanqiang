/**
 * @file blimits.h
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

#ifndef BADVPN_BLIMITS_H
#define BADVPN_BLIMITS_H

#include <stdint.h>

#define BTYPE_IS_SIGNED(type) ((type)-1 < 0)

#define BSIGNED_TYPE_MIN(type) ( \
    sizeof(type) == 1 ? INT8_MIN : ( \
    sizeof(type) == 2 ? INT16_MIN : ( \
    sizeof(type) == 4 ? INT32_MIN : ( \
    sizeof(type) == 8 ? INT64_MIN : 0))))

#define BSIGNED_TYPE_MAX(type) ( \
    sizeof(type) == 1 ? INT8_MAX : ( \
    sizeof(type) == 2 ? INT16_MAX : ( \
    sizeof(type) == 4 ? INT32_MAX : ( \
    sizeof(type) == 8 ? INT64_MAX : 0))))

#define BUNSIGNED_TYPE_MIN(type) ((type)0)

#define BUNSIGNED_TYPE_MAX(type) ( \
    sizeof(type) == 1 ? UINT8_MAX : ( \
    sizeof(type) == 2 ? UINT16_MAX : ( \
    sizeof(type) == 4 ? UINT32_MAX : ( \
    sizeof(type) == 8 ? UINT64_MAX : 0))))

#define BTYPE_MIN(type) (BTYPE_IS_SIGNED(type) ? BSIGNED_TYPE_MIN(type) : BUNSIGNED_TYPE_MIN(type))
#define BTYPE_MAX(type) (BTYPE_IS_SIGNED(type) ? BSIGNED_TYPE_MAX(type) : BUNSIGNED_TYPE_MAX(type))

#endif
