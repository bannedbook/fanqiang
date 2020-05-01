/**
 * @file NCDBuildProgram.c
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
#include <stdlib.h>
#include <string.h>

#include <misc/debug.h>
#include <misc/read_file.h>
#include <misc/strdup.h>
#include <misc/concat_strings.h>
#include <base/BLog.h>
#include <ncd/NCDConfigParser.h>

#include "NCDBuildProgram.h"

#include <generated/blog_channel_NCDBuildProgram.h>

#define MAX_INCLUDE_DEPTH 32

struct guard {
    char *id_data;
    size_t id_length;
    struct guard *next;
};

struct build_state {
    struct guard *top_guard;
};

static int add_guard (struct guard **first, const char *id_data, size_t id_length)
{
    struct guard *g = malloc(sizeof(*g));
    if (!g) {
        goto fail0;
    }
    
    if (!(g->id_data = b_strdup_bin(id_data, id_length))) {
        goto fail1;
    }
    
    g->id_length = id_length;
    
    g->next = *first;
    *first = g;
    
    return 1;
    
fail1:
    free(g);
fail0:
    return 0;
}

static void prepend_guards (struct guard **first, struct guard *guards)
{
    if (!guards) {
        return;
    }
    
    struct guard *last = guards;
    while (last->next) {
        last = last->next;
    }
    
    last->next = *first;
    *first = guards;
}

static void free_guards (struct guard *g)
{
    while (g) {
        struct guard *next_g = g->next;
        free(g->id_data);
        free(g);
        g = next_g;
    }
}

static int guard_exists (struct guard *top_guard, const char *id_data, size_t id_length)
{
    for (struct guard *g = top_guard; g; g = g->next) {
        if (g->id_length == id_length && !memcmp(g->id_data, id_data, id_length)) {
            return 1;
        }
    }
    
    return 0;
}

static char * make_dir_path (const char *file_path)
{
    int found_slash = 0;
    size_t last_slash = 0; // initialize to remove warning
    
    for (size_t i = 0; file_path[i]; i++) {
        if (file_path[i] == '/') {
            found_slash = 1;
            last_slash = i;
        }
    }
    
    char *dir_path;
    
    if (!found_slash) {
        if (!file_path[0]) {
            BLog(BLOG_ERROR, "file '%s': file path must not be empty", file_path);
            return NULL;
        }
        dir_path = b_strdup("");
    } else {
        if (!file_path[last_slash + 1]) {
            BLog(BLOG_ERROR, "file '%s': file path must not end in a slash", file_path);
            return NULL;
        }
        dir_path = b_strdup_bin(file_path, last_slash + 1);
    }
    
    if (!dir_path) {
        BLog(BLOG_ERROR, "file '%s': b_strdup/b_strdup_bin failed", file_path);
        return NULL;
    }
    
    return dir_path;
}

static char * make_include_path (const char *file_path, const char *dir_path, const char *target, size_t target_len)
{
    ASSERT(target_len == strlen(target))
    
    if (target_len == 0) {
        BLog(BLOG_ERROR, "file '%s': include target must not be empty", file_path);
        return NULL;
    }
    
    if (target[target_len - 1] == '/') {
        BLog(BLOG_ERROR, "file '%s': include target must not end in a slash", file_path);
        return NULL;
    }
    
    char *real_target;
    
    if (target[0] == '/') {
        real_target = b_strdup(target);
    } else {
        real_target = concat_strings(2, dir_path, target);
    }
    
    if (!real_target) {
        BLog(BLOG_ERROR, "file '%s': b_strdup/concat_strings failed", file_path);
        return NULL;
    }
    
    return real_target;
}

static int process_file (struct build_state *st, int depth, const char *file_path, NCDProgram *out_program, int *out_guarded)
{
    int ret_val = 0;
    int res;
    
    if (depth > MAX_INCLUDE_DEPTH) {
        BLog(BLOG_ERROR, "file '%s': maximum include depth (%d) exceeded (include cycle?)", file_path, (int)MAX_INCLUDE_DEPTH);
        goto fail0;
    }
    
    char *dir_path = make_dir_path(file_path);
    if (!dir_path) {
        goto fail0;
    }
    
    uint8_t *data;
    size_t len;
    if (!read_file(file_path, &data, &len)) {
        BLog(BLOG_ERROR, "file '%s': failed to read contents", file_path);
        goto fail1;
    }
    
    NCDProgram program;
    res = NCDConfigParser_Parse((char *)data, len, &program);
    free(data);
    if (!res) {
        BLog(BLOG_ERROR, "file '%s': failed to parse", file_path);
        goto fail1;
    }
    
    struct guard *our_guards = NULL;
    
    NCDProgramElem *elem = NCDProgram_FirstElem(&program);
    while (elem) {
        NCDProgramElem *next_elem = NCDProgram_NextElem(&program, elem);
        if (NCDProgramElem_Type(elem) != NCDPROGRAMELEM_INCLUDE_GUARD) {
            elem = next_elem;
            continue;
        }
        
        const char *id_data = NCDProgramElem_IncludeGuardIdData(elem);
        size_t id_length = NCDProgramElem_IncludeGuardIdLength(elem);
        
        if (guard_exists(st->top_guard, id_data, id_length)) {
            *out_guarded = 1;
            ret_val = 1;
            goto fail2;
        }
        
        if (!add_guard(&our_guards, id_data, id_length)) {
            BLog(BLOG_ERROR, "file '%s': add_guard failed", file_path);
            goto fail2;
        }
        
        NCDProgram_RemoveElem(&program, elem);
        elem = next_elem;
    }
    
    prepend_guards(&st->top_guard, our_guards);
    our_guards = NULL;
    
    elem = NCDProgram_FirstElem(&program);
    while (elem) {
        NCDProgramElem *next_elem = NCDProgram_NextElem(&program, elem);
        if (NCDProgramElem_Type(elem) != NCDPROGRAMELEM_INCLUDE) {
            elem = next_elem;
            continue;
        }
        
        const char *target = NCDProgramElem_IncludePathData(elem);
        size_t target_len = NCDProgramElem_IncludePathLength(elem);
        
        if (strlen(target) != target_len) {
            BLog(BLOG_ERROR, "file '%s': include path must not contain null characters", file_path);
            goto fail2;
        }
        
        char *real_target = make_include_path(file_path, dir_path, target, target_len);
        if (!real_target) {
            goto fail2;
        }
        
        NCDProgram included_program;
        int included_guarded;
        res = process_file(st, depth + 1, real_target, &included_program, &included_guarded);
        free(real_target);
        if (!res) {
            goto fail2;
        }
        
        if (included_guarded) {
            NCDProgram_RemoveElem(&program, elem);
        } else {
            if (!NCDProgram_ReplaceElemWithProgram(&program, elem, included_program)) {
                BLog(BLOG_ERROR, "file '%s': NCDProgram_ReplaceElemWithProgram failed", file_path);
                NCDProgram_Free(&included_program);
                goto fail2;
            }
        }
        
        elem = next_elem;
    }
    
    free(dir_path);
    
    *out_program = program;
    *out_guarded = 0;
    return 1;
    
fail2:
    free_guards(our_guards);
    NCDProgram_Free(&program);
fail1:
    free(dir_path);
fail0:
    return ret_val;
}

int NCDBuildProgram_Build (const char *file_path, NCDProgram *out_program)
{
    ASSERT(file_path)
    ASSERT(out_program)
    
    struct build_state st;
    st.top_guard = NULL;
    
    int guarded;
    int res = process_file(&st, 0, file_path, out_program, &guarded);
    
    ASSERT(!res || !guarded)
    
    free_guards(st.top_guard);
    
    return res;
}
