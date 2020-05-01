/**
 * @file basic_functions.c
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

#include <misc/expstring.h>
#include <misc/bsize.h>
#include <misc/ascii_utils.h>
#include <ncd/NCDValGenerator.h>
#include <ncd/NCDValParser.h>
#include <system/BTime.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_basic_functions.h>


// Trivial functions.

static void error_eval (NCDCall call)
{
    FunctionLog(&call, BLOG_ERROR, "error: failing");
}

static void identity_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 1) {
        return FunctionLog(&call, BLOG_ERROR, "identity: need one argument");
    }
    NCDCall_SetResult(&call, NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call)));
}


// Logical functions.

static void if_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 3) {
        return FunctionLog(&call, BLOG_ERROR, "if: need three arguments");
    }
    NCDValRef cond = NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(cond)) {
        return;
    }
    int cond_val;
    if (!ncd_read_boolean(cond, &cond_val)) {
        return FunctionLog(&call, BLOG_ERROR, "if: bad condition");
    }
    int eval_arg = 2 - cond_val;
    NCDCall_SetResult(&call, NCDCall_EvalArg(&call, eval_arg, NCDCall_ResMem(&call)));
}

static void ifel_eval (NCDCall call)
{
    size_t count = NCDCall_ArgCount(&call);
    if (count % 2 == 0) {
        return FunctionLog(&call, BLOG_ERROR, "ifel: need an odd number of arguments");
    }
    NCDValRef value;
    size_t j = 0;
    while (1) {
        NCDValRef arg = NCDCall_EvalArg(&call, j, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            return;
        }
        if (j == count - 1) {
            value = arg;
            break;
        }
        NCDValRef arg2 = NCDCall_EvalArg(&call, j + 1, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg2)) {
            return;
        }
        int arg_val;
        if (!ncd_read_boolean(arg, &arg_val)) {
            return FunctionLog(&call, BLOG_ERROR, "ifel: bad condition");
        }
        if (arg_val) {
            value = arg2;
            break;
        }
        j += 2;
    }
    NCDCall_SetResult(&call, value);
}

static void bool_not_eval (NCDCall call, int negate, char const *name)
{
    if (NCDCall_ArgCount(&call) != 1) {
        return FunctionLog(&call, BLOG_ERROR, "%s: need one argument", name);
    }
    NCDValRef arg = NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(arg)) {
        return;
    }
    int arg_val;
    if (!ncd_read_boolean(arg, &arg_val)) {
        return FunctionLog(&call, BLOG_ERROR, "%s: bad argument", name);
    }
    int res = (arg_val != negate);
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), res));
}

static void bool_eval (NCDCall call) { return bool_not_eval(call, 0, "bool"); }
static void not_eval (NCDCall call) { return bool_not_eval(call, 1, "not"); }

static void and_or_eval (NCDCall call, int is_and, char const *name)
{
    size_t count = NCDCall_ArgCount(&call);
    int res = is_and;
    for (size_t i = 0; i < count; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            return;
        }
        int arg_val;
        if (!ncd_read_boolean(arg, &arg_val)) {
            return FunctionLog(&call, BLOG_ERROR, "%s: bad argument", name);
        }
        if (arg_val != is_and) {
            res = !is_and;
            break;
        }
    }
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), res));
}

static void and_eval (NCDCall call) { return and_or_eval(call, 1, "and"); }
static void or_eval (NCDCall call) { return and_or_eval(call, 0, "or"); }

static void imp_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 2) {
        return FunctionLog(&call, BLOG_ERROR, "imp: need two arguments");
    }
    int res = 0;
    for (size_t i = 0; i < 2; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            return;
        }
        int arg_val;
        if (!ncd_read_boolean(arg, &arg_val)) {
            return FunctionLog(&call, BLOG_ERROR, "imp: bad argument");
        }
        if (arg_val == i) {
            res = 1;
            break;
        }
    }
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), res));
}


// Value comparison functions.

typedef int (*value_compare_func) (int cmp);

static void value_compare_eval (NCDCall call, value_compare_func func)
{
    if (NCDCall_ArgCount(&call) != 2) {
        return FunctionLog(&call, BLOG_ERROR, "value_compare: need two arguments");
    }
    NCDValRef vals[2];
    for (int i = 0; i < 2; i++) {
        vals[i] = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(vals[i])) {
            return;
        }
    }
    int res = func(NCDVal_Compare(vals[0], vals[1]));
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), res));
}

#define DEFINE_VALUE_COMPARE(name, expr) \
static int value_compare_##name##_func (int cmp) { return expr; } \
static void value_compare_##name##_eval (NCDCall call) { return value_compare_eval(call, value_compare_##name##_func); }

DEFINE_VALUE_COMPARE(lesser, (cmp < 0))
DEFINE_VALUE_COMPARE(greater, (cmp > 0))
DEFINE_VALUE_COMPARE(lesser_equal, (cmp <= 0))
DEFINE_VALUE_COMPARE(greater_equal, (cmp >= 0))
DEFINE_VALUE_COMPARE(equal, (cmp == 0))
DEFINE_VALUE_COMPARE(different, (cmp != 0))


// Concatenation functions.

static int concat_recurser (ExpString *estr, NCDValRef arg, NCDCall const *call)
{
    if (NCDVal_IsString(arg)) {
        if (!ExpString_AppendBinaryMr(estr, NCDVal_StringMemRef(arg))) {
            FunctionLog(call, BLOG_ERROR, "ExpString_AppendBinaryMr failed");
            return 0;
        }
    } else if (NCDVal_IsList(arg)) {
        size_t count = NCDVal_ListCount(arg);
        for (size_t i = 0; i < count; i++) {
            if (!concat_recurser(estr, NCDVal_ListGet(arg, i), call)) {
                return 0;
            }
        }
    } else {
        FunctionLog(call, BLOG_ERROR, "concat: value is not a string or list");
        return 0;
    }
    return 1;
}

static void concat_eval (NCDCall call)
{
    ExpString estr;
    if (!ExpString_Init(&estr)) {
        FunctionLog(&call, BLOG_ERROR, "ExpString_Init failed");
        goto fail0;
    }
    size_t count = NCDCall_ArgCount(&call);
    for (size_t i = 0; i < count; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            goto fail1;
        }
        if (!concat_recurser(&estr, arg, &call)) {
            goto fail1;
        }
    }
    NCDCall_SetResult(&call, NCDVal_NewStringBinMr(NCDCall_ResMem(&call), ExpString_GetMr(&estr)));
fail1:
    ExpString_Free(&estr);
fail0:
    return;
}

static void concatlist_eval (NCDCall call)
{
    NCDValRef args_list;
    if (!ncd_eval_func_args(&call, NCDCall_ResMem(&call), &args_list)) {
        return;
    }
    size_t arg_count = NCDVal_ListCount(args_list);
    bsize_t elem_count = bsize_fromsize(0);
    for (size_t i = 0; i < arg_count; i++) {
        NCDValRef arg = NCDVal_ListGet(args_list, i);
        if (!NCDVal_IsList(arg)) {
            return FunctionLog(&call, BLOG_ERROR, "concatlist: argument is not a list");
        }
        elem_count = bsize_add(elem_count, bsize_fromsize(NCDVal_ListCount(arg)));
    }
    if (elem_count.is_overflow) {
        return FunctionLog(&call, BLOG_ERROR, "concatlist: count overflow");
    }
    NCDValRef res = NCDVal_NewList(NCDCall_ResMem(&call), elem_count.value);
    if (NCDVal_IsInvalid(res)) {
        return;
    }
    for (size_t i = 0; i < arg_count; i++) {
        NCDValRef arg = NCDVal_ListGet(args_list, i);
        size_t arg_list_count = NCDVal_ListCount(arg);
        for (size_t j = 0; j < arg_list_count; j++) {
            NCDValRef copy = NCDVal_NewCopy(NCDCall_ResMem(&call), NCDVal_ListGet(arg, j));
            if (NCDVal_IsInvalid(copy)) {
                return;
            }
            if (!NCDVal_ListAppend(res, copy)) {
                return;
            }
        }
    }
    NCDCall_SetResult(&call, res);
}


// Integer comparison functions.

typedef int (*integer_compare_func) (uintmax_t n1, uintmax_t n2);

static void integer_compare_eval (NCDCall call, integer_compare_func func)
{
    if (NCDCall_ArgCount(&call) != 2) {
        return FunctionLog(&call, BLOG_ERROR, "integer_compare: need two arguments");
    }
    uintmax_t ints[2];
    for (int i = 0; i < 2; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            return;
        }
        if (!ncd_read_uintmax(arg, &ints[i])) {
            return FunctionLog(&call, BLOG_ERROR, "integer_compare: wrong value");
        }
    }
    int res = func(ints[0], ints[1]);
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), res));
}

#define DEFINE_INT_COMPARE(name, expr) \
static int integer_compare_##name##_func (uintmax_t n1, uintmax_t n2) { return expr; } \
static void integer_compare_##name##_eval (NCDCall call) { return integer_compare_eval(call, integer_compare_##name##_func); }

DEFINE_INT_COMPARE(lesser, (n1 < n2))
DEFINE_INT_COMPARE(greater, (n1 > n2))
DEFINE_INT_COMPARE(lesser_equal, (n1 <= n2))
DEFINE_INT_COMPARE(greater_equal, (n1 >= n2))
DEFINE_INT_COMPARE(equal, (n1 == n2))
DEFINE_INT_COMPARE(different, (n1 != n2))


// Integer operators.

typedef int (*integer_operator_func) (uintmax_t n1, uintmax_t n2, uintmax_t *out, NCDCall const *call);

static void integer_operator_eval (NCDCall call, integer_operator_func func)
{
    if (NCDCall_ArgCount(&call) != 2) {
        return FunctionLog(&call, BLOG_ERROR, "integer_operator: need two arguments");
    }
    uintmax_t ints[2];
    for (int i = 0; i < 2; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            return;
        }
        if (!ncd_read_uintmax(arg, &ints[i])) {
            return FunctionLog(&call, BLOG_ERROR, "integer_operator: wrong value");
        }
    }
    uintmax_t res;
    if (!func(ints[0], ints[1], &res, &call)) {
        return;
    }
    NCDCall_SetResult(&call, ncd_make_uintmax(NCDCall_ResMem(&call), res));
}

#define DEFINE_INT_OPERATOR(name, expr, check_expr, check_err_str) \
static int integer_operator_##name##_func (uintmax_t n1, uintmax_t n2, uintmax_t *out, NCDCall const *call) \
{ \
    if (check_expr) { \
        FunctionLog(call, BLOG_ERROR, check_err_str); \
        return 0; \
    } \
    *out = expr; \
    return 1; \
} \
static void integer_operator_##name##_eval (NCDCall call) { return integer_operator_eval(call, integer_operator_##name##_func); }

DEFINE_INT_OPERATOR(add, (n1 + n2), (n1 > UINTMAX_MAX - n2), "addition overflow")
DEFINE_INT_OPERATOR(subtract, (n1 - n2), (n1 < n2), "subtraction underflow")
DEFINE_INT_OPERATOR(multiply, (n1 * n2), (n2 != 0 && n1 > UINTMAX_MAX / n2), "multiplication overflow")
DEFINE_INT_OPERATOR(divide, (n1 / n2), (n2 == 0), "division quotient is zero")
DEFINE_INT_OPERATOR(modulo, (n1 % n2), (n2 == 0), "modulo modulus is zero")
DEFINE_INT_OPERATOR(min, (n1 < n2 ? n1 : n2), (0), "")
DEFINE_INT_OPERATOR(max, (n1 > n2 ? n1 : n2), (0), "")


// Encode and decode value.

static void encode_value_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 1) {
        return FunctionLog(&call, BLOG_ERROR, "encode_value: need one argument");
    }
    NCDValRef arg = NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(arg)) {
        return;
    }
    char *str = NCDValGenerator_Generate(arg);
    if (!str) {
        return FunctionLog(&call, BLOG_ERROR, "encode_value: NCDValGenerator_Generate failed");
    }
    NCDCall_SetResult(&call, NCDVal_NewString(NCDCall_ResMem(&call), str));
    free(str);
}

static void decode_value_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 1) {
        return FunctionLog(&call, BLOG_ERROR, "decode_value: need one argument");
    }
    // Evaluate the string to a temporary mem, not ResMem.
    // Otherwise the ResMem could get resized while we're
    // parsing a string within it, and boom.
    NCDValMem temp_mem;
    NCDValMem_Init(&temp_mem, NCDCall_Iparams(&call)->string_index);
    NCDValRef arg = NCDCall_EvalArg(&call, 0, &temp_mem);
    if (NCDVal_IsInvalid(arg)) {
        goto fail1;
    }
    if (!NCDVal_IsString(arg)) {
        FunctionLog(&call, BLOG_ERROR, "decode_value: argument not a string");
        goto fail1;
    }
    NCDValRef value;
    int res = NCDValParser_Parse(NCDVal_StringMemRef(arg), NCDCall_ResMem(&call), &value);
    if (!res) {
        FunctionLog(&call, BLOG_ERROR, "decode_value: NCDValParser_Parse failed");
        goto fail1;
    }
    NCDCall_SetResult(&call, value);
fail1:
    NCDValMem_Free(&temp_mem);
}


// ASCII case conversion

typedef char (*perchar_func) (char ch);

static void perchar_eval (NCDCall call, perchar_func func)
{
    if (NCDCall_ArgCount(&call) != 1) {
        return FunctionLog(&call, BLOG_ERROR, "tolower: need one argument");
    }
    NCDValRef arg = NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(arg)) {
        return;
    }
    if (!NCDVal_IsString(arg)) {
        return FunctionLog(&call, BLOG_ERROR, "tolower: argument not a string");
    }
    NCDValRef value = NCDVal_NewStringUninitialized(NCDCall_ResMem(&call), NCDVal_StringLength(arg));
    if (NCDVal_IsInvalid(value)) {
        return;
    }
    char *out_data = (char *)NCDVal_StringData(value);
    MEMREF_LOOP_CHARS(NCDVal_StringMemRef(arg), i, ch, {
        out_data[i] = func(ch);
    })
    NCDCall_SetResult(&call, value);
}

#define DEFINE_PERCHAR(name, expr) \
static char perchar_##name##_func (char ch) { return expr; } \
static void perchar_##name##_eval (NCDCall call) { return perchar_eval(call, perchar_##name##_func); }

DEFINE_PERCHAR(tolower, b_ascii_tolower(ch))
DEFINE_PERCHAR(toupper, b_ascii_toupper(ch))


// struct_encode, struct_decode

static int read_integer_encoding (NCDValRef encoding, int *out_big, int *out_size)
{
    if (!NCDVal_IsString(encoding)) {
        return 0;
    }
    int big;
    int size;
    if (NCDVal_StringEquals(encoding, "u8")) {
        big = 0;
        size = 1;
    } else if ((big = NCDVal_StringEquals(encoding, "u16b")) || NCDVal_StringEquals(encoding, "u16l")) {
        size = 2;
    } else if ((big = NCDVal_StringEquals(encoding, "u32b")) || NCDVal_StringEquals(encoding, "u32l")) {
        size = 4;
    } else if ((big = NCDVal_StringEquals(encoding, "u64b")) || NCDVal_StringEquals(encoding, "u64l")) {
        size = 8;
    } else {
        return 0;
    }
    *out_big = big;
    *out_size = size;
    return 1;
}

static int struct_encode_single (NCDCall call, ExpString *estr, NCDValRef encoding, NCDValRef value)
{
    uintmax_t val_int;
    if (!ncd_read_uintmax(value, &val_int)) {
        FunctionLog(&call, BLOG_ERROR, "struct_encode: value must be an integer");
        return 0;
    }
    int big;
    int size;
    if (!read_integer_encoding(encoding, &big, &size)) {
        FunctionLog(&call, BLOG_ERROR, "struct_encode: invalid encoding specified");
        return 0;
    }
    uint8_t results[8];
    for (int i = 0; i < size; i++) {
        results[big ? (size - 1 - i) : i] = val_int;
        val_int >>= 8;
    }
    if (val_int > 0) {
        FunctionLog(&call, BLOG_ERROR, "struct_encode: value is out of range");
        return 0;
    }
    if (!ExpString_AppendBinaryMr(estr, MemRef_Make((char const *)results, size))) {
        FunctionLog(&call, BLOG_ERROR, "ExpString_AppendBinaryMr failed");
        return 0;
    }
    return 1;
}

static void struct_encode_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 1) {
        FunctionLog(&call, BLOG_ERROR, "struct_encode: need one argument");
        goto fail0;
    }
    NCDValRef arg = NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(arg)) {
        goto fail0;
    }
    if (!NCDVal_IsList(arg)) {
        FunctionLog(&call, BLOG_ERROR, "struct_encode: argument must be a list");
        goto fail0;
    }
    ExpString estr;
    if (!ExpString_Init(&estr)) {
        FunctionLog(&call, BLOG_ERROR, "ExpString_Init failed");
        goto fail0;
    }
    size_t count = NCDVal_ListCount(arg);
    for (size_t i = 0; i < count; i++) {
        NCDValRef elem = NCDVal_ListGet(arg, i);
        if (!NCDVal_IsList(elem)) {
            FunctionLog(&call, BLOG_ERROR, "struct_encode: element must be a list");
            goto fail1;
        }
        NCDValRef encoding;
        NCDValRef value;
        if (!NCDVal_ListRead(elem, 2, &encoding, &value)) {
            FunctionLog(&call, BLOG_ERROR, "struct_encode: element list must have two elements");
            goto fail1;
        }
        if (!struct_encode_single(call, &estr, encoding, value)) {
            goto fail1;
        }
    }
    NCDCall_SetResult(&call, NCDVal_NewStringBinMr(NCDCall_ResMem(&call), ExpString_GetMr(&estr)));
fail1:
    ExpString_Free(&estr);
fail0:
    return;
}

static int struct_decode_single (NCDCall call, MemRef *data, NCDValRef encoding, NCDValRef result_list)
{
    int big;
    int size;
    if (!read_integer_encoding(encoding, &big, &size)) {
        FunctionLog(&call, BLOG_ERROR, "struct_decode: invalid encoding specified");
        return 0;
    }
    if (data->len < size) {
        FunctionLog(&call, BLOG_ERROR, "struct_decode: insufficient data available");
        return 0;
    }
    uintmax_t val_int = 0;
    for (int i = 0; i < size; i++) {
        val_int <<= 8;
        val_int |= *(uint8_t const *)(data->ptr + (big ? i : (size - 1 - i)));
    }
    *data = MemRef_SubFrom(*data, size);
    NCDValRef value = ncd_make_uintmax(NCDCall_ResMem(&call), val_int);
    if (NCDVal_IsInvalid(value)) {
        return 0;
    }
    if (!NCDVal_ListAppend(result_list, value)) {
        return 0;
    }
    return 1;
}

static void struct_decode_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 2) {
        FunctionLog(&call, BLOG_ERROR, "struct_decode: need two arguments");
        goto fail0;
    }
    NCDValRef format_arg = NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(format_arg)) {
        goto fail0;
    }
    if (!NCDVal_IsList(format_arg)) {
        FunctionLog(&call, BLOG_ERROR, "struct_decode: format argument must be a list");
        goto fail0;
    }
    // Evaluate the data string to temp mem, so the pointer doesn't change.
    NCDValMem temp_mem;
    NCDValMem_Init(&temp_mem, NCDCall_Iparams(&call)->string_index);
    NCDValRef data_arg = NCDCall_EvalArg(&call, 1, &temp_mem);
    if (NCDVal_IsInvalid(data_arg)) {
        goto fail1;
    }
    if (!NCDVal_IsString(data_arg)) {
        FunctionLog(&call, BLOG_ERROR, "struct_decode: data argument must be a string");
        goto fail1;
    }
    size_t count = NCDVal_ListCount(format_arg);
    NCDValRef result_list = NCDVal_NewList(NCDCall_ResMem(&call), count);
    if (NCDVal_IsInvalid(result_list)) {
        goto fail1;
    }
    MemRef data = NCDVal_StringMemRef(data_arg);
    for (size_t i = 0; i < count; i++) {
        NCDValRef encoding = NCDVal_ListGet(format_arg, i);
        if (!struct_decode_single(call, &data, encoding, result_list)) {
            goto fail1;
        }
    }
    if (data.len > 0) {
        FunctionLog(&call, BLOG_ERROR, "struct_decode: not all data was consumed");
        goto fail1;
    }
    NCDCall_SetResult(&call, result_list);
fail1:
    NCDValMem_Free(&temp_mem);
fail0:
    return;
}


// checksum

static void checksum_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 2) {
        FunctionLog(&call, BLOG_ERROR, "checksum: need two arguments");
        return;
    }
    NCDValRef algorithm_arg = NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(algorithm_arg)) {
        return;
    }
    if (!NCDVal_IsString(algorithm_arg)) {
        FunctionLog(&call, BLOG_ERROR, "checksum: algorithm argument must be a string");
        return;
    }
    NCDValRef data_arg = NCDCall_EvalArg(&call, 1, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(data_arg)) {
        return;
    }
    if (!NCDVal_IsString(data_arg)) {
        FunctionLog(&call, BLOG_ERROR, "checksum: data argument must be a string");
        return;
    }
    MemRef data = NCDVal_StringMemRef(data_arg);
    uintmax_t result;
    if (NCDVal_StringEquals(algorithm_arg, "inverted_sum_bytes")) {
        uint8_t s = 0;
        for (size_t i = 0; i < data.len; i++) {
            s += *(uint8_t const *)(data.ptr + i);
        }
        result = (uint8_t)~s;
    } else {
        FunctionLog(&call, BLOG_ERROR, "checksum: unknown algorithm");
        return;
    }
    NCDCall_SetResult(&call, ncd_make_uintmax(NCDCall_ResMem(&call), result));
}


// clock_get_ms

static void clock_get_ms_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 0) {
        FunctionLog(&call, BLOG_ERROR, "clock_get_ms: need zero arguments");
        return;
    }
    // Convert to unsigned. The time should be non-negative so this is OK.
    uintmax_t the_time = btime_gettime();
    NCDCall_SetResult(&call, ncd_make_uintmax(NCDCall_ResMem(&call), the_time));
}


static struct NCDModuleFunction const functions[] = {
    {
        .func_name = "error",
        .func_eval = error_eval
    }, {
        .func_name = "identity",
        .func_eval = identity_eval
    }, {
        .func_name = "if",
        .func_eval = if_eval
    }, {
        .func_name = "ifel",
        .func_eval = ifel_eval
    }, {
        .func_name = "bool",
        .func_eval = bool_eval
    }, {
        .func_name = "not",
        .func_eval = not_eval
    }, {
        .func_name = "and",
        .func_eval = and_eval
    }, {
        .func_name = "or",
        .func_eval = or_eval
    }, {
        .func_name = "imp",
        .func_eval = imp_eval
    }, {
        .func_name = "val_lesser",
        .func_eval = value_compare_lesser_eval
    }, {
        .func_name = "val_greater",
        .func_eval = value_compare_greater_eval
    }, {
        .func_name = "val_lesser_equal",
        .func_eval = value_compare_lesser_equal_eval
    }, {
        .func_name = "val_greater_equal",
        .func_eval = value_compare_greater_equal_eval
    }, {
        .func_name = "val_equal",
        .func_eval = value_compare_equal_eval
    }, {
        .func_name = "val_different",
        .func_eval = value_compare_different_eval
    }, {
        .func_name = "concat",
        .func_eval = concat_eval
    }, {
        .func_name = "concatlist",
        .func_eval = concatlist_eval
    }, {
        .func_name = "num_lesser",
        .func_eval = integer_compare_lesser_eval
    }, {
        .func_name = "num_greater",
        .func_eval = integer_compare_greater_eval
    }, {
        .func_name = "num_lesser_equal",
        .func_eval = integer_compare_lesser_equal_eval
    }, {
        .func_name = "num_greater_equal",
        .func_eval = integer_compare_greater_equal_eval
    }, {
        .func_name = "num_equal",
        .func_eval = integer_compare_equal_eval
    }, {
        .func_name = "num_different",
        .func_eval = integer_compare_different_eval
    }, {
        .func_name = "num_add",
        .func_eval = integer_operator_add_eval
    }, {
        .func_name = "num_subtract",
        .func_eval = integer_operator_subtract_eval
    }, {
        .func_name = "num_multiply",
        .func_eval = integer_operator_multiply_eval
    }, {
        .func_name = "num_divide",
        .func_eval = integer_operator_divide_eval
    }, {
        .func_name = "num_modulo",
        .func_eval = integer_operator_modulo_eval
    }, {
        .func_name = "num_min",
        .func_eval = integer_operator_min_eval
    }, {
        .func_name = "num_max",
        .func_eval = integer_operator_max_eval
    }, {
        .func_name = "encode_value",
        .func_eval = encode_value_eval
    }, {
        .func_name = "decode_value",
        .func_eval = decode_value_eval
    }, {
        .func_name = "tolower",
        .func_eval = perchar_tolower_eval
    }, {
        .func_name = "toupper",
        .func_eval = perchar_toupper_eval
    }, {
        .func_name = "struct_encode",
        .func_eval = struct_encode_eval
    }, {
        .func_name = "struct_decode",
        .func_eval = struct_decode_eval
    }, {
        .func_name = "checksum",
        .func_eval = checksum_eval
    }, {
        .func_name = "clock_get_ms",
        .func_eval = clock_get_ms_eval
    }, {
        .func_name = NULL
    }
};

const struct NCDModuleGroup ncdmodule_basic_functions = {
    .functions = functions
};
