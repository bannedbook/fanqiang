/**
 * @file substring_test.c
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
#include <stdio.h>
#include <time.h>

#include <misc/substring.h>
#include <misc/debug.h>
#include <misc/balloc.h>
#include <misc/print_macros.h>

static int find_substring_slow (const char *str, size_t str_len, const char *sub, size_t sub_len, size_t *out_pos)
{
    ASSERT(sub_len > 0)
    
    if (str_len < sub_len) {
        return 0;
    }
    
    for (size_t i = 0; i <= str_len - sub_len; i++) {
        if (!memcmp(str + i, sub, sub_len)) {
            *out_pos = i;
            return 1;
        }
    }
    
    return 0;
}

static void print_data (const char *str, size_t str_len)
{
    while (str_len > 0) {
        printf("%02"PRIx8" ", (uint8_t)(*str));
        str++;
        str_len--;
    }
    printf("\n");
}

static void print_table (const size_t *table, size_t len)
{
    for (size_t i = 1; i < len; i++) {
        printf("%zu ", table[i]);
    }
    printf("\n");
}

static void test_tables (int len, int count)
{
    ASSERT(len > 0)
    ASSERT(count >= 0)
    
    char *word = (char *)BAllocSize(bsize_fromint(len));
    ASSERT_FORCE(word)
    
    size_t *table = (size_t *)BAllocSize(bsize_mul(bsize_fromint(len), bsize_fromsize(sizeof(table[0]))));
    ASSERT_FORCE(table)
    
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < len; j++) {
            word[j] = rand() % 2;
        }
        
        build_substring_backtrack_table(MemRef_Make(word, len), table);
        
        for (int j = 1; j < len; j++) {
            for (int k = j - 1; k >= 0; k--) {
                if (!memcmp(word + j - k, word, k)) {
                    ASSERT_FORCE(table[j] == k)
                    break;
                }
            }
        }
    }
    
    BFree(table);
    BFree(word);
}

static void test_substring (int word_len, int text_len, int word_count, int text_count)
{
    assert(word_len > 0);
    assert(text_len >= 0);
    assert(word_count >= 0);
    assert(text_count >= 0);
    
    char *word = (char *)BAllocSize(bsize_fromint(word_len));
    ASSERT_FORCE(word)
    
    size_t *table = (size_t *)BAllocSize(bsize_mul(bsize_fromint(word_len), bsize_fromsize(sizeof(table[0]))));
    ASSERT_FORCE(table)
    
    char *text = (char *)BAllocSize(bsize_fromint(text_len));
    ASSERT_FORCE(text)
    
    for (int i = 0; i < word_count; i++) {
        for (int j = 0; j < word_len; j++) {
            word[j] = rand() % 2;
        }
        
        build_substring_backtrack_table(MemRef_Make(word, word_len), table);
        
        for (int j = 0; j < text_count; j++) {
            for (int k = 0; k < text_len; k++) {
                text[k] = rand() % 2;
            }
            
            size_t pos = 36; // to remove warning
            int res = find_substring(MemRef_Make(text, text_len), MemRef_Make(word, word_len), table, &pos);
            
            size_t spos = 59; // to remove warning
            int sres = find_substring_slow(text, text_len, word, word_len, &spos);
            
            ASSERT_FORCE(res == sres)
            if (res) {
                ASSERT_FORCE(pos == spos)
            }
        }
    }
    
    BFree(text);
    BFree(table);
    BFree(word);
}

int main (int argc, char *argv[])
{
    if (argc != 7) {
        printf("Usage: %s <tables length> <tables count> <word len> <text len> <word count> <text count>\n", (argc == 0 ? "" : argv[0]));
        return 1;
    }
    
    int tables_len = atoi(argv[1]);
    int tables_count = atoi(argv[2]);
    int word_len = atoi(argv[3]);
    int text_len = atoi(argv[4]);
    int word_count = atoi(argv[5]);
    int text_count = atoi(argv[6]);
    
    if (tables_len <= 0 || tables_count < 0 || word_len <= 0 || text_len < 0 || word_count < 0 || text_count < 0) {
        printf("Bad arguments.\n");
        return 1;
    }
    
    srand(time(NULL));
    
    test_tables(tables_len, tables_count);
    
    test_substring(word_len, text_len, word_count, text_count);
    
    {
        char text[] = "aggagaa";
        char word[] = "aga";
        size_t table[sizeof(word) - 1];
        build_substring_backtrack_table(MemRef_MakeCstr(word), table);
        
        size_t pos;
        int res = find_substring(MemRef_MakeCstr(text), MemRef_MakeCstr(word), table, &pos);
        ASSERT_FORCE(res)
        ASSERT_FORCE(pos == 3)
    }
    
    {
        char text[] = "aagagga";
        char word[] = "aga";
        size_t table[sizeof(word) - 1];
        build_substring_backtrack_table_reverse(MemRef_MakeCstr(word), table);
        
        size_t pos;
        int res = find_substring_reverse(MemRef_MakeCstr(text), MemRef_MakeCstr(word), table, &pos);
        ASSERT_FORCE(res)
        ASSERT_FORCE(pos == 1)
    }
    
    return 0;
}
