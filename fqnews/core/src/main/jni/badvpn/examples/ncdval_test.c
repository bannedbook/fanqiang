/**
 * @file ncdval_test.c
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

#include <stdio.h>

#include <ncd/NCDVal.h>
#include <ncd/NCDStringIndex.h>
#include <ncd/static_strings.h>
#include <base/BLog.h>
#include <misc/debug.h>
#include <misc/balloc.h>
#include <misc/offset.h>

#define FORCE(cmd) if (!(cmd)) { fprintf(stderr, "failed\n"); exit(1); }

static void test_string (NCDValRef str, const char *data, size_t length)
{
    FORCE( !NCDVal_IsInvalid(str) )
    FORCE( NCDVal_IsString(str) )
    FORCE( NCDVal_StringLength(str) == length )
    FORCE( NCDVal_StringHasNulls(str) == !!memchr(data, '\0', length) )
    FORCE( NCDVal_IsStringNoNulls(str) == !memchr(data, '\0', length) )
    FORCE( NCDVal_StringRegionEquals(str, 0, length, data) )
    
    MemRef mr = NCDVal_StringMemRef(str);
    FORCE( mr.ptr == NCDVal_StringData(str) )
    FORCE( mr.len == NCDVal_StringLength(str) )
    FORCE( mr.len == length )
    FORCE( !memcmp(mr.ptr, data, length) )
}

static void print_indent (int indent)
{
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static void print_value (NCDValRef val, unsigned int indent)
{
    switch (NCDVal_Type(val)) {
        case NCDVAL_STRING: {
            NCDValNullTermString nts;
            FORCE( NCDVal_StringNullTerminate(val, &nts) )
            
            print_indent(indent);
            printf("string(%zu) %s\n", NCDVal_StringLength(val), nts.data);
            
            NCDValNullTermString_Free(&nts);
        } break;
        
        case NCDVAL_LIST: {
            size_t count = NCDVal_ListCount(val);
            
            print_indent(indent);
            printf("list(%zu)\n", NCDVal_ListCount(val));
            
            for (size_t i = 0; i < count; i++) {
                NCDValRef elem_val = NCDVal_ListGet(val, i);
                print_value(elem_val, indent + 1);
            }
        } break;
        
        case NCDVAL_MAP: {
            print_indent(indent);
            printf("map(%zu)\n", NCDVal_MapCount(val));
            
            for (NCDValMapElem e = NCDVal_MapOrderedFirst(val); !NCDVal_MapElemInvalid(e); e = NCDVal_MapOrderedNext(val, e)) {
                NCDValRef ekey = NCDVal_MapElemKey(val, e);
                NCDValRef eval = NCDVal_MapElemVal(val, e);
                
                print_indent(indent + 1);
                printf("key=\n");
                print_value(ekey, indent + 2);
                
                print_indent(indent + 1);
                printf("val=\n");
                print_value(eval, indent + 2);
            }
        } break;
    }
}

int main ()
{
    int res;
    
    BLog_InitStdout();
    
    NCDStringIndex string_index;
    FORCE( NCDStringIndex_Init(&string_index) )
    
    // Some basic usage of values.
    
    NCDValMem mem;
    NCDValMem_Init(&mem, &string_index);
    
    NCDValRef s1 = NCDVal_NewString(&mem, "Hello World");
    test_string(s1, "Hello World", 11);
    ASSERT( NCDVal_IsString(s1) )
    ASSERT( !NCDVal_IsIdString(s1) )
    ASSERT( NCDVal_Type(s1) == NCDVAL_STRING )
    
    NCDValRef s2 = NCDVal_NewString(&mem, "This is reeeeeeeeeeeeeallllllllyyyyy fun!");
    FORCE( !NCDVal_IsInvalid(s2) )
    
    NCDValRef l1 = NCDVal_NewList(&mem, 10);
    FORCE( !NCDVal_IsInvalid(l1) )
    
    FORCE( NCDVal_ListAppend(l1, s1) )
    FORCE( NCDVal_ListAppend(l1, s2) )
    
    print_value(s1, 0);
    print_value(s2, 0);
    print_value(l1, 0);
    
    NCDValRef k1 = NCDVal_NewString(&mem, "K1");
    FORCE( !NCDVal_IsInvalid(k1) )
    NCDValRef v1 = NCDVal_NewString(&mem, "V1");
    FORCE( !NCDVal_IsInvalid(v1) )
    
    NCDValRef k2 = NCDVal_NewString(&mem, "K2");
    FORCE( !NCDVal_IsInvalid(k2) )
    NCDValRef v2 = NCDVal_NewString(&mem, "V2");
    FORCE( !NCDVal_IsInvalid(v2) )
    
    NCDValRef m1 = NCDVal_NewMap(&mem, 3);
    FORCE( !NCDVal_IsInvalid(m1) )
    
    FORCE( NCDVal_MapInsert(m1, k1, v1, &res) && res )
    FORCE( NCDVal_MapInsert(m1, k2, v2, &res) && res )
    
    ASSERT( NCDVal_MapGetValue(m1, "K1").idx == v1.idx )
    ASSERT( NCDVal_MapGetValue(m1, "K2").idx == v2.idx )
    ASSERT( NCDVal_IsInvalid(NCDVal_MapGetValue(m1, "K3")) )
    
    NCDValRef ids1 = NCDVal_NewIdString(&mem, NCD_STRING_ARG1);
    test_string(ids1, "_arg1", 5);
    ASSERT( !memcmp(NCDVal_StringData(ids1), "_arg1", 5) )
    ASSERT( NCDVal_StringLength(ids1) == 5 )
    ASSERT( !NCDVal_StringHasNulls(ids1) )
    ASSERT( NCDVal_StringEquals(ids1, "_arg1") )
    ASSERT( NCDVal_Type(ids1) == NCDVAL_STRING )
    ASSERT( NCDVal_IsIdString(ids1) )
    
    NCDValRef ids2 = NCDVal_NewIdString(&mem, NCD_STRING_ARG2);
    test_string(ids2, "_arg2", 5);
    ASSERT( !memcmp(NCDVal_StringData(ids2), "_arg2", 5) )
    ASSERT( NCDVal_StringLength(ids2) == 5 )
    ASSERT( !NCDVal_StringHasNulls(ids2) )
    ASSERT( NCDVal_StringEquals(ids2, "_arg2") )
    ASSERT( NCDVal_Type(ids2) == NCDVAL_STRING )
    ASSERT( NCDVal_IsIdString(ids2) )
    
    FORCE( NCDVal_MapInsert(m1, ids1, ids2, &res) && res )
    
    ASSERT( NCDVal_MapGetValue(m1, "_arg1").idx == ids2.idx )
    
    print_value(m1, 0);
    
    NCDValRef copy = NCDVal_NewCopy(&mem, m1);
    FORCE( !NCDVal_IsInvalid(copy) )
    ASSERT( NCDVal_Compare(copy, m1) == 0 )
    
    NCDValMem_Free(&mem);
    
    // Try to make copies of a string within the same memory object.
    // This is an evil test because we cannot simply copy a string using e.g.
    // NCDVal_NewStringBin() - it requires that the buffer passed
    // be outside the memory object of the new string.
    // We use NCDVal_NewCopy(), which takes care of this by creating
    // an uninitialized string using NCDVal_NewStringUninitialized() and
    // then copyng the data.
    
    NCDValMem_Init(&mem, &string_index);
    
    NCDValRef s[100];
    
    s[0] = NCDVal_NewString(&mem, "Eeeeeeeeeeeevil.");
    FORCE( !NCDVal_IsInvalid(s[0]) )
    
    for (int i = 1; i < 100; i++) {
        s[i] = NCDVal_NewCopy(&mem, s[i - 1]);
        FORCE( !NCDVal_IsInvalid(s[i]) )
        ASSERT( NCDVal_StringEquals(s[i - 1], "Eeeeeeeeeeeevil.") )
        ASSERT( NCDVal_StringEquals(s[i], "Eeeeeeeeeeeevil.") )
    }
    
    for (int i = 0; i < 100; i++) {
        ASSERT( NCDVal_StringEquals(s[i], "Eeeeeeeeeeeevil.") )
    }
    
    NCDValMem_Free(&mem);
    
    NCDStringIndex_Free(&string_index);
    
    return 0;
}
