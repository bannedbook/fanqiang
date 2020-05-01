/**
 * @file strdup.h
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
 * Allocate memory for a string and copy it there.
 */

#ifndef BADVPN_STRDUP_H
#define BADVPN_STRDUP_H

#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <misc/debug.h>

/**
 * Allocate and copy a null-terminated string.
 */
static char * b_strdup (const char *str)
{
    ASSERT(str)
    
    size_t len = strlen(str);
    
    char *s = (char *)malloc(len + 1);
    if (!s) {
        return NULL;
    }
    
    memcpy(s, str, len + 1);
    
    return s;
}

/**
 * Allocate memory for a null-terminated string and use the
 * given data as its contents. A null terminator is appended
 * after the specified data.
 */
static char * b_strdup_bin (const char *str, size_t len)
{
    ASSERT(str)
    
    if (len == SIZE_MAX) {
        return NULL;
    }
    
    char *s = (char *)malloc(len + 1);
    if (!s) {
        return NULL;
    }
    
    memcpy(s, str, len);
    s[len] = '\0';
    
    return s;
}

#endif
