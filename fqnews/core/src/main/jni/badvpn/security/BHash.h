/**
 * @file BHash.h
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
 * Cryptographic hash funtions abstraction.
 */

#ifndef BADVPN_SECURITY_BHASH_H
#define BADVPN_SECURITY_BHASH_H

#include <stdint.h>

#include <openssl/md5.h>
#include <openssl/sha.h>

#include <misc/debug.h>

#define BHASH_TYPE_MD5 1
#define BHASH_TYPE_MD5_SIZE 16

#define BHASH_TYPE_SHA1 2
#define BHASH_TYPE_SHA1_SIZE 20

#define BHASH_MAX_SIZE 20

/**
 * Checks if the given hash type number is valid.
 * 
 * @param type hash type number
 * @return 1 if valid, 0 if not
 */
int BHash_type_valid (int type);

/**
 * Returns the size of a hash.
 * 
 * @param cipher hash type number. Must be valid.
 * @return hash size in bytes
 */
int BHash_size (int type);

/**
 * Calculates a hash.
 * {@link BSecurity_GlobalInitThreadSafe} must have been done if this is
 * being called from a non-main thread.
 * 
 * @param type hash type number. Must be valid.
 * @param data data to calculate the hash of
 * @param data_len length of data
 * @param out the hash will be written here. Must not overlap with data.
 */
void BHash_calculate (int type, uint8_t *data, int data_len, uint8_t *out);

#endif
