#include <stddef.h>

#include <misc/debug.h>
#include <misc/memref.h>

static void build_substring_backtrack_table (MemRef str, size_t *out_table)
{
    ASSERT(str.len > 0)
    
    size_t x = 0;
    
    for (size_t i = 1; i < str.len; i++) {
        out_table[i] = x;
        while (x > 0 && str.ptr[i] != str.ptr[x]) {
            x = out_table[x];
        }
        if (str.ptr[i] == str.ptr[x]) {
            x++;
        }
    }
}

static int find_substring (MemRef text, MemRef word, const size_t *table, size_t *out_position)
{
    ASSERT(word.len > 0)
    
    size_t x = 0;
    
    for (size_t i = 0; i < text.len; i++) {
        while (x > 0 && text.ptr[i] != word.ptr[x]) {
            x = table[x];
        }
        if (text.ptr[i] == word.ptr[x]) {
            if (x + 1 == word.len) {
                *out_position = i - x;
                return 1;
            }
            x++;
        }
    }
    
    return 0;
}

static void build_substring_backtrack_table_reverse (MemRef str, size_t *out_table)
{
    ASSERT(str.len > 0)
    
    size_t x = 0;
    
    for (size_t i = 1; i < str.len; i++) {
        out_table[i] = x;
        while (x > 0 && str.ptr[str.len - 1 - i] != str.ptr[str.len - 1 - x]) {
            x = out_table[x];
        }
        if (str.ptr[str.len - 1 - i] == str.ptr[str.len - 1 - x]) {
            x++;
        }
    }
}

static int find_substring_reverse (MemRef text, MemRef word, const size_t *table, size_t *out_position)
{
    ASSERT(word.len > 0)
    
    size_t x = 0;
    
    for (size_t i = 0; i < text.len; i++) {
        while (x > 0 && text.ptr[text.len - 1 - i] != word.ptr[word.len - 1 - x]) {
            x = table[x];
        }
        if (text.ptr[text.len - 1 - i] == word.ptr[word.len - 1 - x]) {
            if (x + 1 == word.len) {
                *out_position = (text.len - 1 - i);
                return 1;
            }
            x++;
        }
    }
    
    return 0;
}
