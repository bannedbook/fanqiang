/**
 * @file value_utils.h
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

#ifndef NCD_VALUE_UTILS_H
#define NCD_VALUE_UTILS_H

#include <stdint.h>
#include <stddef.h>

#include <system/BTime.h>
#include <ncd/NCDVal.h>
#include <ncd/NCDStringIndex.h>
#include <ncd/NCDModule.h>

int ncd_is_none (NCDValRef val);
NCDValRef ncd_make_boolean (NCDValMem *mem, int value);
int ncd_read_boolean (NCDValRef val, int *out) WARN_UNUSED;
int ncd_read_uintmax (NCDValRef string, uintmax_t *out) WARN_UNUSED;
int ncd_read_time (NCDValRef string, btime_t *out) WARN_UNUSED;
NCD_string_id_t ncd_get_string_id (NCDValRef string);
NCDValRef ncd_make_uintmax (NCDValMem *mem, uintmax_t value);
char * ncd_strdup (NCDValRef stringnonulls);
int ncd_eval_func_args_ext (NCDCall const *call, size_t start, size_t count, NCDValMem *mem, NCDValRef *out) WARN_UNUSED;
int ncd_eval_func_args (NCDCall const *call, NCDValMem *mem, NCDValRef *out) WARN_UNUSED;

#endif
