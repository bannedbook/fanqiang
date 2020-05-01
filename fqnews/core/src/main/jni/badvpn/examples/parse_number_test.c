/**
 * @file parse_number_test.c
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
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <misc/parse_number.h>
#include <misc/debug.h>

static void test_random (int num_digits, int digit_modulo)
{
    ASSERT(num_digits > 0);
    ASSERT(digit_modulo > 0);
    
    uint8_t digits[40];
    
    for (int i = 0; i < num_digits; i++) {
        digits[i] = '0' + (rand() % digit_modulo);
    }
    digits[num_digits] = '\0';
    
    char *endptr;
    uintmax_t std_num = strtoumax((const char *)digits, &endptr, 10);
    int std_res = !*endptr && !(std_num == UINTMAX_MAX && errno == ERANGE);
    
    uintmax_t num = 0;
    int res = parse_unsigned_integer(MemRef_Make((const char *)digits, num_digits), &num);
    
    if (res != std_res) {
        printf("fail1 %s\n", (const char *)digits);
        ASSERT_FORCE(0);
    }
    
    if (res && num != std_num) {
        printf("fail2 %s\n", (const char *)digits);
        ASSERT_FORCE(0);
    }
    
    if (res) {
        uint8_t *nozero_digits = digits;
        while (*nozero_digits == '0' && nozero_digits != &digits[num_digits - 1]) {
            nozero_digits++;
        }
        
        char buf[40];
        int size = compute_decimal_repr_size(num);
        generate_decimal_repr(num, buf, size);
        buf[size] = '\0';
        ASSERT_FORCE(!strcmp(buf, (const char *)nozero_digits));
    }
}

static void test_value (uintmax_t x)
{
    char str[40];
    sprintf(str, "%" PRIuMAX, x);
    uintmax_t y;
    int res = parse_unsigned_integer(MemRef_MakeCstr(str), &y);
    ASSERT_FORCE(res);
    ASSERT_FORCE(y == x);
    
    char str2[40];
    int size = compute_decimal_repr_size(x);
    generate_decimal_repr(x, str2, size);
    str2[size] = '\0';
    
    ASSERT_FORCE(!strcmp(str2, str));
}

static void test_value_range (uintmax_t start, uintmax_t count)
{
    uintmax_t i = start;
    do {
        test_value(i);
        i++;
    } while (i != start + count);
}

int main ()
{
    srand(time(NULL));
    
    for (int num_digits = 1; num_digits <= 22; num_digits++) {
        for (int i = 0; i < 1000000; i++) {
            test_random(num_digits, 10);
        }
        for (int i = 0; i < 1000000; i++) {
            test_random(num_digits, 11);
        }
    }
    
    test_value_range(UINTMAX_C(0), 5000000);
    test_value_range(UINTMAX_C(100000000), 5000000);
    test_value_range(UINTMAX_C(258239003), 5000000);
    test_value_range(UINTMAX_C(8241096180752634), 5000000);
    test_value_range(UINTMAX_C(9127982390882308083), 5000000);
    test_value_range(UINTMAX_C(18446744073700000000), 20000000);
    
    return 0;
}
