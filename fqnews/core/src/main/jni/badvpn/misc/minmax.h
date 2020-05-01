/**
 * @file minmax.h
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
 * Minimum and maximum macros.
 */

#ifndef BADVPN_MISC_MINMAX_H
#define BADVPN_MISC_MINMAX_H

#include <stddef.h>
#include <stdint.h>

#define DEFINE_BMINMAX(name, type) \
static type bmin ## name (type a, type b) { return (a < b ? a : b); } \
static type bmax ## name (type a, type b) { return (a > b ? a : b); }

DEFINE_BMINMAX(_size, size_t)
DEFINE_BMINMAX(_int, int)
DEFINE_BMINMAX(_int8, int8_t)
DEFINE_BMINMAX(_int16, int16_t)
DEFINE_BMINMAX(_int32, int32_t)
DEFINE_BMINMAX(_int64, int64_t)
DEFINE_BMINMAX(_uint, unsigned int)
DEFINE_BMINMAX(_uint8, uint8_t)
DEFINE_BMINMAX(_uint16, uint16_t)
DEFINE_BMINMAX(_uint32, uint32_t)
DEFINE_BMINMAX(_uint64, uint64_t)

#endif
