/**
 * @file read_file.h
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
 * Function for reading a file into memory using stdio.
 */

#ifndef BADVPN_MISC_READ_FILE_H
#define BADVPN_MISC_READ_FILE_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

static int read_file (const char *file, uint8_t **out_data, size_t *out_len)
{
    FILE *f = fopen(file, "r");
    if (!f) {
        goto fail0;
    }
    
    size_t buf_len = 0;
    size_t buf_size = 128;
    
    uint8_t *buf = (uint8_t *)malloc(buf_size);
    if (!buf) {
        goto fail1;
    }
    
    while (1) {
        if (buf_len == buf_size) {
            if (2 > SIZE_MAX / buf_size) {
                goto fail;
            }
            size_t newsize = 2 * buf_size;
            
            uint8_t *newbuf = (uint8_t *)realloc(buf, newsize);
            if (!newbuf) {
                goto fail;
            }
            
            buf = newbuf;
            buf_size = newsize;
        }
        
        size_t bytes = fread(buf + buf_len, 1, buf_size - buf_len, f);
        if (bytes == 0) {
            if (feof(f)) {
                break;
            }
            goto fail;
        }
        
        buf_len += bytes;
    }
    
    fclose(f);
    
    *out_data = buf;
    *out_len = buf_len;
    return 1;
    
fail:
    free(buf);
fail1:
    fclose(f);
fail0:
    return 0;
}

#endif
