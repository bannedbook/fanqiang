/**
 * @file file.c
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
 * File I/O module.
 * 
 * Synopsis:
 *   file_read(string filename)
 * 
 * Variables:
 *   string (empty) - file contents
 * 
 * Description:
 *   Reads the contents of a file. Reports an error if something goes wrong.
 *   WARNING: this uses fopen/fread/fclose, blocking the entire interpreter while
 *            the file is being read. For this reason, you should only use this
 *            to read small local files which will be read quickly, and especially
 *            not files on network mounts.
 * 
 * Synopsis:
 *   file_write(string filename, string contents)
 * 
 * Description:
 *   Writes a file, possibly overwriting an existing one. Reports an error if something
 *   goes wrong.
 *   WARNING: this is not an atomic operation; other programs may see the file in an
 *            inconsistent state while it is being written. Similarly, if writing
 *            fails, the file may remain in an inconsistent state indefinitely.
 *            If this is a problem, you should write the new contents to a temporary
 *            file and rename this temporary file to the live file.
 *   WARNING: this uses fopen/fwrite/fclose, blocking the entire interpreter while
 *            the file is being written. For this reason, you should only use this
 *            to write small local files which will be written quickly, and especially
 *            not files on network mounts.
 * 
 * Synopsis:
 *   file_stat(string filename)
 *   file_lstat(string filename)
 * 
 * Description:
 *   Retrieves information about a file.
 *   file_stat() follows symlinks; file_lstat() does not and allows retrieving information
 *   about a symlink.
 *   WARNING: this blocks the interpreter
 * 
 * Variables:
 *   succeeded - whether the stat operation succeeded (true/false). If false, all other
 *               variables obtain the value "failed".
 *   type - file, dir, chr, blk, fifo, link, socket, other, failed
 *   size - size of the file, or failed
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <misc/read_file.h>
#include <misc/write_file.h>
#include <misc/parse_number.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_file.h>

struct read_instance {
    NCDModuleInst *i;
    uint8_t *file_data;
    size_t file_len;
};

struct stat_instance {
    NCDModuleInst *i;
    int succeeded;
    struct stat result;
};

static void read_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct read_instance *o = vo;
    o->i = i;
    
    // read arguments
    NCDValRef filename_arg;
    if (!NCDVal_ListRead(params->args, 1, &filename_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(filename_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // get null terminated name
    NCDValNullTermString filename_nts;
    if (!NCDVal_StringNullTerminate(filename_arg, &filename_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    // read file
    int res = read_file(filename_nts.data, &o->file_data, &o->file_len);
    NCDValNullTermString_Free(&filename_nts);
    if (!res) {
        ModuleLog(i, BLOG_ERROR, "failed to read file");
        goto fail0;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void read_func_die (void *vo)
{
    struct read_instance *o = vo;
    
    // free data
    free(o->file_data);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int read_func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct read_instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        *out = NCDVal_NewStringBin(mem, o->file_data, o->file_len);
        return 1;
    }
    
    return 0;
}

static void write_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // read arguments
    NCDValRef filename_arg;
    NCDValRef contents_arg;
    if (!NCDVal_ListRead(params->args, 2, &filename_arg, &contents_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(filename_arg) || !NCDVal_IsString(contents_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // get null terminated name
    NCDValNullTermString filename_nts;
    if (!NCDVal_StringNullTerminate(filename_arg, &filename_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    // write file
    int res = write_file(filename_nts.data, NCDVal_StringMemRef(contents_arg));
    NCDValNullTermString_Free(&filename_nts);
    if (!res) {
        ModuleLog(i, BLOG_ERROR, "failed to write file");
        goto fail0;
    }
    
    // signal up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void stat_func_new_common (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int is_lstat)
{
    struct stat_instance *o = vo;
    o->i = i;
    
    NCDValRef filename_arg;
    if (!NCDVal_ListRead(params->args, 1, &filename_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(filename_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    o->succeeded = 0;
    
    if (!NCDVal_IsStringNoNulls(filename_arg)) {
        goto out;
    }
    
    // null terminate filename
    NCDValNullTermString filename_nts;
    if (!NCDVal_StringNullTerminate(filename_arg, &filename_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail0;
    }
    
    int res;
    if (is_lstat) {
        res = lstat(filename_nts.data, &o->result);
    } else {
        res = stat(filename_nts.data, &o->result);
    }
    NCDValNullTermString_Free(&filename_nts);
    
    if (res < 0) {
        goto out;
    }
    
    o->succeeded = 1;
    
out:
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void stat_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    stat_func_new_common(vo, i, params, 0);
}

static void lstat_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    stat_func_new_common(vo, i, params, 1);
}

static int stat_func_getvar2 (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct stat_instance *o = vo;
    
    if (name == NCD_STRING_SUCCEEDED) {
        *out = ncd_make_boolean(mem, o->succeeded);
        return 1;
    }
    
    if (name == NCD_STRING_TYPE) {
        const char *str;
        
        if (!o->succeeded) {
            str = "failed";
        } else if (S_ISREG(o->result.st_mode)) {
            str = "file";
        } else if (S_ISDIR(o->result.st_mode)) {
            str = "dir";
        } else if (S_ISCHR(o->result.st_mode)) {
            str = "chr";
        } else if (S_ISBLK(o->result.st_mode)) {
            str = "blk";
        } else if (S_ISFIFO(o->result.st_mode)) {
            str = "fifo";
        } else if (S_ISLNK(o->result.st_mode)) {
            str = "link";
        } else if (S_ISSOCK(o->result.st_mode)) {
            str = "socket";
        } else {
            str = "other";
        }
        
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    if (name == NCD_STRING_SIZE) {
        char str[50];
        if (!o->succeeded) {
            strcpy(str, "failed");
        } else {
            generate_decimal_repr_string((uintmax_t)o->result.st_size, str);
        }
        
        *out = NCDVal_NewString(mem, str);
        return 1;
    }
    
    return 0;
}

static struct NCDModule modules[] = {
    {
        .type = "file_read",
        .func_new2 = read_func_new,
        .func_die = read_func_die,
        .func_getvar2 = read_func_getvar2,
        .alloc_size = sizeof(struct read_instance)
    }, {
        .type = "file_write",
        .func_new2 = write_func_new
    }, {
        .type = "file_stat",
        .func_new2 = stat_func_new,
        .func_getvar2 = stat_func_getvar2,
        .alloc_size = sizeof(struct stat_instance)
    }, {
        .type = "file_lstat",
        .func_new2 = lstat_func_new,
        .func_getvar2 = stat_func_getvar2,
        .alloc_size = sizeof(struct stat_instance)
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_file = {
    .modules = modules
};
