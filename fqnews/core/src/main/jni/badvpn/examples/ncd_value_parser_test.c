/**
 * @file ncd_value_parser_test.c
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <misc/debug.h>
#include <base/BLog.h>
#include <ncd/NCDValParser.h>
#include <ncd/NCDValGenerator.h>

int main (int argc, char *argv[])
{
    int res = 1;
    
    if (argc != 2) {
        printf("Usage: %s <string>\n", (argc > 0 ? argv[0] : ""));
        goto fail0;
    }
    
    BLog_InitStdout();
    
    NCDStringIndex string_index;
    if (!NCDStringIndex_Init(&string_index)) {
        DEBUG("NCDStringIndex_Init failed");
        goto fail01;
    }
    
    NCDValMem mem;
    NCDValMem_Init(&mem, &string_index);
    
    // parse
    NCDValRef val;
    if (!NCDValParser_Parse(MemRef_MakeCstr(argv[1]), &mem, &val)) {
        DEBUG("NCDValParser_Parse failed");
        goto fail1;
    }
    
    // generate value string
    char *str = NCDValGenerator_Generate(val);
    if (!str) {
        DEBUG("NCDValGenerator_Generate failed");
        goto fail1;
    }
    
    // print value string
    printf("%s\n", str);
    
    res = 0;
    
    free(str);
fail1:
    NCDValMem_Free(&mem);
    NCDStringIndex_Free(&string_index);
fail01:
    BLog_Free();
fail0:
    return res;
}
