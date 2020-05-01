/**
 * @file find_program.h
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
 * Function that finds the absolute path of a program by checking a predefined
 * list of directories.
 */

#ifndef BADVPN_FIND_PROGRAM_H
#define BADVPN_FIND_PROGRAM_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <misc/concat_strings.h>
#include <misc/debug.h>
#include <misc/balloc.h>

static char * badvpn_find_program (const char *name);

static char * badvpn_find_program (const char *name)
{
    ASSERT(name)
    
    char *path = getenv("PATH");
    if (path) {
        while (1) {
            size_t i = 0;
            while (path[i] != ':' && path[i] != '\0') {
                i++;
            }
            char const *src = path;
            size_t src_len = i;
            if (src_len == 0) {
                src = ".";
                src_len = 1;
            }
            size_t name_len = strlen(name);
            char *entry = BAllocSize(bsize_add(bsize_fromsize(src_len), bsize_add(bsize_fromsize(name_len), bsize_fromsize(2))));
            if (!entry) {
                goto fail;
            }
            memcpy(entry, src, src_len);
            entry[src_len] = '/';
            strcpy(entry + (src_len + 1), name);
            if (access(entry, X_OK) == 0) {
                return entry;
            }
            free(entry);
            if (path[i] == '\0') {
                break;
            }
            path += i + 1;
        }
    }
    
    const char *dirs[] = {"/usr/sbin", "/usr/bin", "/sbin", "/bin", NULL};
    
    for (size_t i = 0; dirs[i]; i++) {
        char *try_path = concat_strings(3, dirs[i], "/", name);
        if (!try_path) {
            goto fail;
        }
        
        if (access(try_path, X_OK) == 0) {
            return try_path;
        }
        
        free(try_path);
    }
    
fail:
    return NULL;
}

#endif
