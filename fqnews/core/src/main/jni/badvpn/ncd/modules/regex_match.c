/**
 * @file regex_match.c
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
 * Regular expression matching module.
 * 
 * Synopsis:
 *   regex_match(string input, string regex)
 * 
 * Variables:
 *   succeeded - "true" or "false", indicating whether input matched regex
 *   matchN - for N=0,1,2,..., the matching data for the N-th subexpression
 *     (match0 = whole match)
 * 
 * Description:
 *   Matches 'input' with the POSIX extended regular expression 'regex'.
 *   'regex' must be a string without null bytes, but 'input' can contain null bytes.
 *   However, it's difficult, if not impossible, to actually match nulls with the regular
 *   expression.
 *   The input and regex strings are interpreted according to the POSIX regex functions
 *   (regcomp(), regexec()); in particular, the current locale setting affects the
 *   interpretation.
 * 
 * Synopsis:
 *   regex_replace(string input, list(string) regex, list(string) replace)
 * 
 * Variables:
 *   string (empty) - transformed input
 * 
 * Description:
 *   Replaces matching parts of a string. Replacement is performed by repetedly matching
 *   the remaining part of the string with all regular expressions. On each step, out of
 *   all regular expressions that match the remainder of the string, the one whose match
 *   starts at the least position wins, and the matching part is replaced with the
 *   replacement string corresponding to this regular expression. The process continues
 *   from the end of the just-replaced portion until no more regular expressions match.
 *   If multiple regular expressions match at the least position, the one that appears
 *   first in the 'regex' argument wins.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <regex.h>

#include <misc/string_begins_with.h>
#include <misc/parse_number.h>
#include <misc/expstring.h>
#include <misc/debug.h>
#include <misc/balloc.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_regex_match.h>

#define MAX_MATCHES 64

struct instance {
    NCDModuleInst *i;
    MemRef input;
    int succeeded;
    int num_matches;
    regmatch_t matches[MAX_MATCHES];
};

struct replace_instance {
    NCDModuleInst *i;
    MemRef output;
};

static void func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef input_arg;
    NCDValRef regex_arg;
    if (!NCDVal_ListRead(params->args, 2, &input_arg, &regex_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(input_arg) || !NCDVal_IsStringNoNulls(regex_arg)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    o->input = NCDVal_StringMemRef(input_arg);
    
    // make sure we don't overflow regoff_t
    if (o->input.len > INT_MAX) {
        ModuleLog(o->i, BLOG_ERROR, "input string too long");
        goto fail0;
    }
    
    // null terminate regex
    NCDValNullTermString regex_nts;
    if (!NCDVal_StringNullTerminate(regex_arg, &regex_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    // compile regex
    regex_t preg;
    int ret = regcomp(&preg, regex_nts.data, REG_EXTENDED);
    NCDValNullTermString_Free(&regex_nts);
    if (ret != 0) {
        ModuleLog(o->i, BLOG_ERROR, "regcomp failed (error=%d)", ret);
        goto fail0;
    }
    
    // execute match
    o->matches[0].rm_so = 0;
    o->matches[0].rm_eo = o->input.len;
    o->succeeded = (regexec(&preg, o->input.ptr, MAX_MATCHES, o->matches, REG_STARTEND) == 0);
    
    // free regex
    regfree(&preg);
    
    // signal up
    NCDModuleInst_Backend_Up(o->i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static int func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct instance *o = vo;
    
    if (!strcmp(name, "succeeded")) {
        *out = ncd_make_boolean(mem, o->succeeded);
        return 1;
    }
    
    size_t pos;
    uintmax_t n;
    if ((pos = string_begins_with(name, "match")) && parse_unsigned_integer(MemRef_MakeCstr(name + pos), &n)) {
        if (o->succeeded && n < MAX_MATCHES && o->matches[n].rm_so >= 0) {
            regmatch_t *m = &o->matches[n];
            
            ASSERT(m->rm_so <= o->input.len)
            ASSERT(m->rm_eo >= m->rm_so)
            ASSERT(m->rm_eo <= o->input.len)
            
            size_t len = m->rm_eo - m->rm_so;
            
            *out = NCDVal_NewStringBinMr(mem, MemRef_Sub(o->input, m->rm_so, len));
            return 1;
        }
    }
    
    return 0;
}

static void replace_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct replace_instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef input_arg;
    NCDValRef regex_arg;
    NCDValRef replace_arg;
    if (!NCDVal_ListRead(params->args, 3, &input_arg, &regex_arg, &replace_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail1;
    }
    if (!NCDVal_IsString(input_arg) || !NCDVal_IsList(regex_arg) || !NCDVal_IsList(replace_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail1;
    }
    
    // check number of regex/replace
    if (NCDVal_ListCount(regex_arg) != NCDVal_ListCount(replace_arg)) {
        ModuleLog(i, BLOG_ERROR, "number of regex's is not the same as number of replacements");
        goto fail1;
    }
    size_t num_regex = NCDVal_ListCount(regex_arg);
    
    // allocate array for compiled regex's
    regex_t *regs = BAllocArray(num_regex, sizeof(regs[0]));
    if (!regs) {
        ModuleLog(i, BLOG_ERROR, "BAllocArray failed");
        goto fail1;
    }
    size_t num_done_regex = 0;
    
    // compile regex's, check arguments
    while (num_done_regex < num_regex) {
        NCDValRef regex = NCDVal_ListGet(regex_arg, num_done_regex);
        NCDValRef replace = NCDVal_ListGet(replace_arg, num_done_regex);
        
        if (!NCDVal_IsStringNoNulls(regex) || !NCDVal_IsString(replace)) {
            ModuleLog(i, BLOG_ERROR, "wrong regex/replace type for pair %zu", num_done_regex);
            goto fail2;
        }
        
        // null terminate regex
        NCDValNullTermString regex_nts;
        if (!NCDVal_StringNullTerminate(regex, &regex_nts)) {
            ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
            goto fail2;
        }
        
        int res = regcomp(&regs[num_done_regex], regex_nts.data, REG_EXTENDED);
        NCDValNullTermString_Free(&regex_nts);
        if (res != 0) {
            ModuleLog(i, BLOG_ERROR, "regcomp failed for pair %zu (error=%d)", num_done_regex, res);
            goto fail2;
        }
        
        num_done_regex++;
    }
    
    // init output string
    ExpString out;
    if (!ExpString_Init(&out)) {
        ModuleLog(i, BLOG_ERROR, "ExpString_Init failed");
        goto fail2;
    }
    
    // input state
    MemRef in = NCDVal_StringMemRef(input_arg);
    size_t in_pos = 0;
    
    // process input
    while (in_pos < in.len) {
        // find first match
        int have_match = 0;
        size_t match_regex = 0; // to remove warning
        regmatch_t match = {0, 0}; // to remove warning
        for (size_t j = 0; j < num_regex; j++) {
            regmatch_t this_match;
            this_match.rm_so = 0;
            this_match.rm_eo = in.len - in_pos;
            if (regexec(&regs[j], in.ptr + in_pos, 1, &this_match, REG_STARTEND) == 0 && (!have_match || this_match.rm_so < match.rm_so)) {
                have_match = 1;
                match_regex = j;
                match = this_match;
            }
        }
        
        // if no match, append remaining data and finish
        if (!have_match) {
            if (!ExpString_AppendBinaryMr(&out, MemRef_SubFrom(in, in_pos))) {
                ModuleLog(i, BLOG_ERROR, "ExpString_AppendBinaryMr failed");
                goto fail3;
            }
            break;
        }
        
        // append data before match
        if (!ExpString_AppendBinaryMr(&out, MemRef_Sub(in, in_pos, match.rm_so))) {
            ModuleLog(i, BLOG_ERROR, "ExpString_AppendBinaryMr failed");
            goto fail3;
        }
        
        // append replacement data
        NCDValRef replace = NCDVal_ListGet(replace_arg, match_regex);
        if (!ExpString_AppendBinaryMr(&out, NCDVal_StringMemRef(replace))) {
            ModuleLog(i, BLOG_ERROR, "ExpString_AppendBinaryMr failed");
            goto fail3;
        }
        
        in_pos += match.rm_eo;
    }
    
    // set output
    o->output = ExpString_GetMr(&out);
    
    // free compiled regex's
    while (num_done_regex-- > 0) {
        regfree(&regs[num_done_regex]);
    }
    
    // free array
    BFree(regs);
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail3:
    ExpString_Free(&out);
fail2:
    while (num_done_regex-- > 0) {
        regfree(&regs[num_done_regex]);
    }
    BFree(regs);
fail1:
    NCDModuleInst_Backend_DeadError(i);
}

static void replace_func_die (void *vo)
{
    struct replace_instance *o = vo;
    
    // free output
    BFree((char *)o->output.ptr);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int replace_func_getvar (void *vo, const char *name, NCDValMem *mem, NCDValRef *out)
{
    struct replace_instance *o = vo;
    
    if (!strcmp(name, "")) {
        *out = NCDVal_NewStringBinMr(mem, o->output);
        return 1;
    }
    
    return 0;
}

static struct NCDModule modules[] = {
    {
        .type = "regex_match",
        .func_new2 = func_new,
        .func_getvar = func_getvar,
        .alloc_size = sizeof(struct instance)
    }, {
        .type = "regex_replace",
        .func_new2 = replace_func_new,
        .func_die = replace_func_die,
        .func_getvar = replace_func_getvar,
        .alloc_size = sizeof(struct replace_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_regex_match = {
    .modules = modules
};
