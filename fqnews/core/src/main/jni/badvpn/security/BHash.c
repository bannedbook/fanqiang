/**
 * @file BHash.c
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

#include <security/BHash.h>

int BHash_type_valid (int type)
{
    switch (type) {
        case BHASH_TYPE_MD5:
        case BHASH_TYPE_SHA1:
            return 1;
        default:
            return 0;
    }
}

int BHash_size (int type)
{
    switch (type) {
        case BHASH_TYPE_MD5:
            return BHASH_TYPE_MD5_SIZE;
        case BHASH_TYPE_SHA1:
            return BHASH_TYPE_SHA1_SIZE;
        default:
            ASSERT(0)
            return 0;
    }
}

void BHash_calculate (int type, uint8_t *data, int data_len, uint8_t *out)
{
    switch (type) {
        case BHASH_TYPE_MD5:
            MD5(data, data_len, out);
            break;
        case BHASH_TYPE_SHA1:
            SHA1(data, data_len, out);
            break;
        default:
            ASSERT(0)
            ;
    }
}
