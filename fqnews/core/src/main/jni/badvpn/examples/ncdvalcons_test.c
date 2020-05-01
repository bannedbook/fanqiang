/**
 * @file ncdvalcons_test.c
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

#include <string.h>
#include <stdio.h>

#include <misc/debug.h>
#include <ncd/NCDValCons.h>
#include <ncd/NCDValGenerator.h>

static NCDStringIndex string_index;
static NCDValMem mem;
static NCDValCons cons;

static NCDValConsVal make_string (const char *data)
{
    NCDValConsVal val;
    int error;
    int res = NCDValCons_NewString(&cons, (const uint8_t *)data, strlen(data), &val, &error);
    ASSERT_FORCE(res)
    return val;
}

static NCDValConsVal make_list (void)
{
    NCDValConsVal val;
    NCDValCons_NewList(&cons, &val);
    return val;
}

static NCDValConsVal make_map (void)
{
    NCDValConsVal val;
    NCDValCons_NewMap(&cons, &val);
    return val;
}

static NCDValConsVal list_prepend (NCDValConsVal list, NCDValConsVal elem)
{
    int error;
    int res = NCDValCons_ListPrepend(&cons, &list, elem, &error);
    ASSERT_FORCE(res)
    return list;
}

static NCDValConsVal map_insert (NCDValConsVal map, NCDValConsVal key, NCDValConsVal value)
{
    int error;
    int res = NCDValCons_MapInsert(&cons, &map, key, value, &error);
    ASSERT_FORCE(res)
    return map;
}

static NCDValRef complete (NCDValConsVal cval)
{
    int error;
    NCDValRef val;
    int res = NCDValCons_Complete(&cons, cval, &val, &error);
    ASSERT_FORCE(res)
    return val;
}

int main ()
{
    int res;
    
    res = NCDStringIndex_Init(&string_index);
    ASSERT_FORCE(res)
    
    NCDValMem_Init(&mem, &string_index);
    
    res = NCDValCons_Init(&cons, &mem);
    ASSERT_FORCE(res)
    
    NCDValRef val1 = complete(list_prepend(list_prepend(list_prepend(make_list(), make_string("hello")), make_string("world")), make_list()));
    char *str1 = NCDValGenerator_Generate(val1);
    ASSERT_FORCE(str1)
    ASSERT_FORCE(!strcmp(str1, "{{}, \"world\", \"hello\"}"))
    free(str1);
    
    NCDValRef val2 = complete(map_insert(map_insert(map_insert(make_map(), make_list(), make_list()), make_string("A"), make_list()), make_string("B"), make_list()));
    char *str2 = NCDValGenerator_Generate(val2);
    ASSERT_FORCE(str2)
    printf("%s\n", str2);
    ASSERT_FORCE(!strcmp(str2, "[\"A\":{}, \"B\":{}, {}:{}]"))
    free(str2);
    
    NCDValCons_Free(&cons);
    NCDValMem_Free(&mem);
    NCDStringIndex_Free(&string_index);
    return 0;
}
