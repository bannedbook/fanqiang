/**
 * @file NCDValGenerator.c
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

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <misc/debug.h>
#include <misc/expstring.h>
#include <base/BLog.h>

#include "NCDValGenerator.h"

#include <generated/blog_channel_NCDValGenerator.h>

static int generate_val (NCDValRef value, ExpString *out_str)
{
    ASSERT(!NCDVal_IsInvalid(value))
    
    switch (NCDVal_Type(value)) {
        case NCDVAL_STRING: {
            if (!ExpString_AppendChar(out_str, '"')) {
                BLog(BLOG_ERROR, "ExpString_AppendChar failed");
                goto fail;
            }
            
            MEMREF_LOOP_CHARS(NCDVal_StringMemRef(value), char_pos, ch, {
                if (ch == '\0') {
                    char buf[5];
                    snprintf(buf, sizeof(buf), "\\x%02"PRIx8, (uint8_t)ch);
                    
                    if (!ExpString_Append(out_str, buf)) {
                        BLog(BLOG_ERROR, "ExpString_Append failed");
                        goto fail;
                    }
                    
                    goto next_char;
                }
                
                if (ch == '"' || ch == '\\') {
                    if (!ExpString_AppendChar(out_str, '\\')) {
                        BLog(BLOG_ERROR, "ExpString_AppendChar failed");
                        goto fail;
                    }
                }
                
                if (!ExpString_AppendChar(out_str, ch)) {
                    BLog(BLOG_ERROR, "ExpString_AppendChar failed");
                    goto fail;
                }
            next_char:;
            })
            
            if (!ExpString_AppendChar(out_str, '"')) {
                BLog(BLOG_ERROR, "ExpString_AppendChar failed");
                goto fail;
            }
        } break;
        
        case NCDVAL_LIST: {
            size_t count = NCDVal_ListCount(value);
            
            if (!ExpString_AppendChar(out_str, '{')) {
                BLog(BLOG_ERROR, "ExpString_AppendChar failed");
                goto fail;
            }
            
            for (size_t i = 0; i < count; i++) {
                if (i > 0) {
                    if (!ExpString_Append(out_str, ", ")) {
                        BLog(BLOG_ERROR, "ExpString_Append failed");
                        goto fail;
                    }
                }
                
                if (!generate_val(NCDVal_ListGet(value, i), out_str)) {
                    goto fail;
                }
            }
            
            if (!ExpString_AppendChar(out_str, '}')) {
                BLog(BLOG_ERROR, "ExpString_AppendChar failed");
                goto fail;
            }
        } break;
        
        case NCDVAL_MAP: {
            if (!ExpString_AppendChar(out_str, '[')) {
                BLog(BLOG_ERROR, "ExpString_AppendChar failed");
                goto fail;
            }
            
            int is_first = 1;
            
            for (NCDValMapElem e = NCDVal_MapOrderedFirst(value); !NCDVal_MapElemInvalid(e); e = NCDVal_MapOrderedNext(value, e)) {
                NCDValRef ekey = NCDVal_MapElemKey(value, e);
                NCDValRef eval = NCDVal_MapElemVal(value, e);
                
                if (!is_first) {
                    if (!ExpString_Append(out_str, ", ")) {
                        BLog(BLOG_ERROR, "ExpString_Append failed");
                        goto fail;
                    }
                }
                
                if (!generate_val(ekey, out_str)) {
                    goto fail;
                }
                
                if (!ExpString_AppendChar(out_str, ':')) {
                    BLog(BLOG_ERROR, "ExpString_AppendChar failed");
                    goto fail;
                }
                
                if (!generate_val(eval, out_str)) {
                    goto fail;
                }
                
                is_first = 0;
            }
            
            if (!ExpString_AppendChar(out_str, ']')) {
                BLog(BLOG_ERROR, "ExpString_AppendChar failed");
                goto fail;
            }
        } break;
        
        default: ASSERT(0);
    }
    
    return 1;
    
fail:
    return 0;
}

char * NCDValGenerator_Generate (NCDValRef value)
{
    ASSERT(!NCDVal_IsInvalid(value))
    
    ExpString str;
    if (!ExpString_Init(&str)) {
        BLog(BLOG_ERROR, "ExpString_Init failed");
        goto fail0;
    }
    
    if (!generate_val(value, &str)) {
        goto fail1;
    }
    
    return ExpString_Get(&str);
    
fail1:
    ExpString_Free(&str);
fail0:
    return NULL;
}

int NCDValGenerator_AppendGenerate (NCDValRef value, ExpString *str)
{
    ASSERT(!NCDVal_IsInvalid(value))
    ASSERT(str)
    
    return generate_val(value, str);
}
