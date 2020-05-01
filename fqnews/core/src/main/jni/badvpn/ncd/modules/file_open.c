/**
 * @file file_open.c
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
 * Synopsis:
 *   file_open(string filename, string mode [, map options])
 * 
 * Variables:
 *   string is_error - "true" if the file_open object is in error state, "false"
 *     otherwise
 * 
 * Options:
 *   "read_size" - the maximum number of bytes that can be read by a single
 *     read() call. Must be greater than zero. Greater values may improve
 *     performance, but will increase memory usage. Default: 8192.
 * 
 * Description:
 *   Opens a file for subsequent reading or writing. The 'mode' argument must
 *   be one of: "r", "w", "a", "r+", "w+", "a+"; it corresponds to the mode string
 *   that will be passed to the fopen() function.
 *   When the file_open() statement goes up, the error state is set depending on
 *   whether opening succeeded or failed. The 'is_error' variable should be used
 *   to check the error state.
 *   If an error occurs afterward within read(), write() or seek(), the error state
 *   is set, and the file_open() statement is toggled down and back up. This way,
 *   the same piece of user code can handle all file errors.
 * 
 * Synopsis:
 *   file_open::read()
 * 
 * Variables:
 *   string (empty) - the data which was read, or an empty string if EOF was reached
 *   string not_eof - "false" if EOF was reached, "true" if not
 * 
 * Description:
 *   Reads data from an opened file. The file must not be in error state.
 *   If reading fails, this statement will never go up, the error state of the
 *   file_open() statement will be set, and the file_open() statement will trigger
 *   backtracking (go down and up).
 * 
 * Synopsis:
 *   file_open::write(string data)
 * 
 * Description:
 *   Writes data to an opened file. The file must not be in error state.
 *   If writing fails, this statement will never go up, the error state of the
 *   file_open() statement will be set, and the file_open() statement will trigger
 *   backtracking (go down and up).
 * 
 * Synopsis:
 *   file_open::seek(string position, string whence)
 * 
 * Description:
 *   Sets the file position indicator. The 'position' argument must be a possibly
 *   negative decimal number, and is interpreted relative to 'whence'. Here, 'whence'
 *   may be one of:
 *   - "set", meaning beginning of file,
 *   - "cur", meaning the current position, and
 *   - "end", meaning the end of file.
 *   Errors are handled as in read() and write(). Note that if the position argument
 *   is too small or too large to convert to off_t, this is not a seek error, and only
 *   the seek command will fail.
 * 
 * Synopsis:
 *   file_open::close()
 * 
 * Description:
 *   Closes the file. The file must not be in error state.
 *   Errors are handled as handled as in read() and write(), i.e. the process is
 *   backtracked to file_open() with the error state set.
 *   On success, the error state of the file is set (but without backtracking), and
 *   the close() statement goes up .
 */

#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include <misc/debug.h>
#include <misc/balloc.h>
#include <misc/parse_number.h>
#include <ncd/extra/NCDBuf.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_file_open.h>

#define READ_BUF_SIZE 8192

struct open_instance {
    NCDModuleInst *i;
    FILE *fh;
    NCDBufStore store;
};

struct read_instance {
    NCDModuleInst *i;
    NCDBuf *buf;
    size_t length;
};

static int parse_mode (MemRef mr, char *out)
{
    size_t pos = 0;
    size_t left = mr.len;
    
    if (left == 0) {
        return 0;
    }
    switch (MemRef_At(mr, pos)) {
        case 'r':
        case 'w':
        case 'a':
            *out++ = MemRef_At(mr, pos);
            pos++;
            left--;
            break;
        default:
            return 0;
    }
    
    if (left == 0) {
        goto finish;
    }
    switch (MemRef_At(mr, pos)) {
        case '+':
            *out++ = MemRef_At(mr, pos);
            pos++;
            left--;
            break;
        default:
            return 0;
    }
    
    if (left == 0) {
        goto finish;
    }
    
    return 0;
    
finish:
    *out = '\0';
    return 1;
}

static void trigger_error (struct open_instance *o)
{
    if (o->fh) {
        // close file
        if (fclose(o->fh) != 0) {
            ModuleLog(o->i, BLOG_ERROR, "fclose failed");
        }
        
        // set no file, indicating error
        o->fh = NULL;
    }
    
    // go down and up
    NCDModuleInst_Backend_Down(o->i);
    NCDModuleInst_Backend_Up(o->i);
}

static void open_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct open_instance *o = vo;
    o->i = i;
    
    // check arguments
    NCDValRef filename_arg;
    NCDValRef mode_arg;
    NCDValRef options_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 2, &filename_arg, &mode_arg) &&
        !NCDVal_ListRead(params->args, 3, &filename_arg, &mode_arg, &options_arg)
    ) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsStringNoNulls(filename_arg) || !NCDVal_IsString(mode_arg) ||
        (!NCDVal_IsInvalid(options_arg) && !NCDVal_IsMap(options_arg))
    ) {
        ModuleLog(o->i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // check mode
    char mode[5];
    if (!parse_mode(NCDVal_StringMemRef(mode_arg), mode)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong mode");
        goto fail0;
    }
    
    size_t read_size_opt = READ_BUF_SIZE;
    
    // parse options
    if (!NCDVal_IsInvalid(options_arg)) {
        int num_recognized = 0;
        NCDValRef value;
        
        if (!NCDVal_IsInvalid(value = NCDVal_MapGetValue(options_arg, "read_size"))) {
            uintmax_t read_size;
            if (!ncd_read_uintmax(value, &read_size) || read_size > SIZE_MAX || read_size == 0) {
                ModuleLog(o->i, BLOG_ERROR, "wrong read_size");
                goto fail0;
            }
            num_recognized++;
            read_size_opt = read_size;
        }
        
        if (NCDVal_MapCount(options_arg) > num_recognized) {
            ModuleLog(o->i, BLOG_ERROR, "unrecognized options present");
            goto fail0;
        }
    }
    
    // init store
    NCDBufStore_Init(&o->store, read_size_opt);
    
    // null terminate filename
    NCDValNullTermString filename_nts;
    if (!NCDVal_StringNullTerminate(filename_arg, &filename_nts)) {
        ModuleLog(i, BLOG_ERROR, "NCDVal_StringNullTerminate failed");
        goto fail1;
    }
    
    // open file
    o->fh = fopen(filename_nts.data, mode);
    NCDValNullTermString_Free(&filename_nts);
    if (!o->fh) {
        ModuleLog(o->i, BLOG_ERROR, "fopen failed");
    }
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail1:
    NCDBufStore_Free(&o->store);
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void open_func_die (void *vo)
{
    struct open_instance *o = vo;
    
    // close file
    if (o->fh) {
        if (fclose(o->fh) != 0) {
            ModuleLog(o->i, BLOG_ERROR, "fclose failed");
        }
    }
    
    // free store
    NCDBufStore_Free(&o->store);
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int open_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct open_instance *o = vo;
    
    if (name == NCD_STRING_IS_ERROR) {
        *out = ncd_make_boolean(mem, !o->fh);
        return 1;
    }
    
    return 0;
}

static void read_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct read_instance *o = vo;
    o->i = i;
    
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(o->i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get open instance
    struct open_instance *open_inst = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // make sure it's not in error
    if (!open_inst->fh) {
        ModuleLog(o->i, BLOG_ERROR, "open instance is in error");
        goto fail0;
    }
    
    // get buffer
    o->buf = NCDBufStore_GetBuf(&open_inst->store);
    if (!o->buf) {
        ModuleLog(o->i, BLOG_ERROR, "NCDBufStore_GetBuf failed");
        goto fail0;
    }
    
    // starting with empty buffer
    char *data = NCDBuf_Data(o->buf);
    size_t buf_size = NCDBufStore_BufSize(&open_inst->store);
    o->length = 0;
    
    while (o->length < buf_size) {
        // read
        size_t readed = fread(data + o->length, 1, buf_size - o->length, open_inst->fh);
        if (readed == 0) {
            break;
        }
        ASSERT(readed <= buf_size - o->length)
        
        // increment length
        o->length += readed;
    }
    
    // if we couldn't read anything due to an error, trigger
    // error in the open instance, and don't go up
    if (o->length == 0 && !feof(open_inst->fh)) {
        ModuleLog(o->i, BLOG_ERROR, "fread failed");
        trigger_error(open_inst);
        return;
    }
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void read_func_die (void *vo)
{
    struct read_instance *o = vo;
    
    // release buffer
    BRefTarget_Deref(NCDBuf_RefTarget(o->buf));
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int read_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct read_instance *o = vo;
    
    if (name == NCD_STRING_EMPTY) {
        *out = NCDVal_NewExternalString(mem, NCDBuf_Data(o->buf), o->length, NCDBuf_RefTarget(o->buf));
        return 1;
    }
    
    if (name == NCD_STRING_NOT_EOF) {
        *out = ncd_make_boolean(mem, (o->length != 0));
        return 1;
    }
    
    return 0;
}

static void write_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    NCDValRef data_arg;
    if (!NCDVal_ListRead(params->args, 1, &data_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(data_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // get open instance
    struct open_instance *open_inst = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // make sure it's not in error
    if (!open_inst->fh) {
        ModuleLog(i, BLOG_ERROR, "open instance is in error");
        goto fail0;
    }
    
    // write all the data
    MemRef data_mr = NCDVal_StringMemRef(data_arg);
    size_t pos = 0;
    while (pos < data_mr.len) {
        size_t written = fwrite(data_mr.ptr + pos, 1, data_mr.len - pos, open_inst->fh);
        if (written == 0) {
            ModuleLog(i, BLOG_ERROR, "fwrite failed");
            trigger_error(open_inst);
            return;
        }
        ASSERT(written <= data_mr.len - pos)
        pos += written;
    }
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void seek_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    NCDValRef position_arg;
    NCDValRef whence_arg;
    if (!NCDVal_ListRead(params->args, 2, &position_arg, &whence_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(position_arg) || !NCDVal_IsString(whence_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    // parse position
    int position_sign;
    uintmax_t position_mag;
    if (!parse_signmag_integer(NCDVal_StringMemRef(position_arg), &position_sign, &position_mag)) {
        ModuleLog(i, BLOG_ERROR, "wrong position");
        goto fail0;
    }
    
    // parse whence
    int whence;
    if (NCDVal_StringEquals(whence_arg, "set")) {
        whence = SEEK_SET;
    }
    else if (NCDVal_StringEquals(whence_arg, "cur")) {
        whence = SEEK_CUR;
    }
    else if (NCDVal_StringEquals(whence_arg, "end")) {
        whence = SEEK_END;
    }
    else {
        ModuleLog(i, BLOG_ERROR, "wrong whence");
        goto fail0;
    }
    
    // determine min/max values of off_t (non-portable hack)
    off_t off_t_min = (sizeof(off_t) == 8 ? INT64_MIN : INT32_MIN);
    off_t off_t_max = (sizeof(off_t) == 8 ? INT64_MAX : INT32_MAX);
    
    // compute position as off_t
    off_t position;
    if (position_sign < 0 && position_mag > 0) {
        if (position_mag - 1 > -(off_t_min + 1)) {
            ModuleLog(i, BLOG_ERROR, "position underflow");
            goto fail0;
        }
        position = -(off_t)(position_mag - 1) - 1;
    } else {
        if (position_mag > off_t_max) {
            ModuleLog(i, BLOG_ERROR, "position overflow");
            goto fail0;
        }
        position = position_mag;
    }
    
    // get open instance
    struct open_instance *open_inst = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // make sure it's not in error
    if (!open_inst->fh) {
        ModuleLog(i, BLOG_ERROR, "open instance is in error");
        goto fail0;
    }
    
    // seek
    if (fseeko(open_inst->fh, position, whence) < 0) {
        ModuleLog(i, BLOG_ERROR, "fseeko failed");
        trigger_error(open_inst);
        return;
    }
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void close_func_new (void *unused, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    // check arguments
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // get open instance
    struct open_instance *open_inst = NCDModuleInst_Backend_GetUser((NCDModuleInst *)params->method_user);
    
    // make sure it's not in error
    if (!open_inst->fh) {
        ModuleLog(i, BLOG_ERROR, "open instance is in error");
        goto fail0;
    }
    
    // close
    int res = fclose(open_inst->fh);
    open_inst->fh = NULL;
    if (res != 0) {
        ModuleLog(i, BLOG_ERROR, "fclose failed");
        trigger_error(open_inst);
        return;
    }
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "file_open",
        .func_new2 = open_func_new,
        .func_die = open_func_die,
        .func_getvar2 = open_func_getvar,
        .alloc_size = sizeof(struct open_instance)
    }, {
        .type = "file_open::read",
        .func_new2 = read_func_new,
        .func_die = read_func_die,
        .func_getvar2 = read_func_getvar,
        .alloc_size = sizeof(struct read_instance)
    }, {
        .type = "file_open::write",
        .func_new2 = write_func_new
    }, {
        .type = "file_open::seek",
        .func_new2 = seek_func_new
    }, {
        .type = "file_open::close",
        .func_new2 = close_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_file_open = {
    .modules = modules
};
