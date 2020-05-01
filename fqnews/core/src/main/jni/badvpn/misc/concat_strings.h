/**
 * @file concat_strings.h
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
 * Function for concatenating strings.
 */

#ifndef BADVPN_MISC_CONCAT_STRINGS_H
#define BADVPN_MISC_CONCAT_STRINGS_H

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <misc/debug.h>

static char * concat_strings (int num, ...)
{
    ASSERT(num >= 0)
    
    // calculate sum of lengths
    size_t sum = 0;
    va_list ap;
    va_start(ap, num);
    for (int i = 0; i < num; i++) {
        const char *str = va_arg(ap, const char *);
        size_t str_len = strlen(str);
        if (str_len > SIZE_MAX - 1 - sum) {
            va_end(ap);
            return NULL;
        }
        sum += str_len;
    }
    va_end(ap);
    
    // allocate memory
    char *res_str = (char *)malloc(sum + 1);
    if (!res_str) {
        return NULL;
    }
    
    // copy strings
    sum = 0;
    va_start(ap, num);
    for (int i = 0; i < num; i++) {
        const char *str = va_arg(ap, const char *);
        size_t str_len = strlen(str);
        memcpy(res_str + sum, str, str_len);
        sum += str_len;
    }
    va_end(ap);
    
    // terminate
    res_str[sum] = '\0';
    
    return res_str;
}

#endif
