/**
 * @file string_begins_with.h
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
 * Function for checking if a string begins with a given string.
 */

#ifndef BADVPN_MISC_STRING_BEGINS_WITH
#define BADVPN_MISC_STRING_BEGINS_WITH

#include <stddef.h>
#include <string.h>

#include <misc/debug.h>

static size_t data_begins_with (const char *str, size_t str_len, const char *needle)
{
    ASSERT(strlen(needle) > 0)
    
    size_t len = 0;
    
    while (str_len > 0 && *needle) {
        if (*str != *needle) {
            return 0;
        }
        str++;
        str_len--;
        needle++;
        len++;
    }
    
    if (*needle) {
        return 0;
    }
    
    return len;
}

static size_t string_begins_with (const char *str, const char *needle)
{
    ASSERT(strlen(needle) > 0)
    
    return data_begins_with(str, strlen(str), needle);
}

static size_t data_begins_with_bin (const char *str, size_t str_len, const char *needle, size_t needle_len)
{
    ASSERT(needle_len > 0)
    
    size_t len = 0;
    
    while (str_len > 0 && needle_len > 0) {
        if (*str != *needle) {
            return 0;
        }
        str++;
        str_len--;
        needle++;
        needle_len--;
        len++;
    }
    
    if (needle_len > 0) {
        return 0;
    }
    
    return len;
}

#endif
