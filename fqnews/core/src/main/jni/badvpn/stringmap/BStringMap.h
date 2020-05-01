/**
 * @file BStringMap.h
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

#ifndef BADVPN_STRINGMAP_BSTRINGMAP_H
#define BADVPN_STRINGMAP_BSTRINGMAP_H

#include <misc/debug.h>
#include <structure/BAVL.h>
#include <base/DebugObject.h>

struct BStringMap_entry {
    char *key;
    char *value;
    BAVLNode tree_node;
};

typedef struct {
    BAVL tree;
    DebugObject d_obj;
} BStringMap;

void BStringMap_Init (BStringMap *o);
int BStringMap_InitCopy (BStringMap *o, const BStringMap *src) WARN_UNUSED;
void BStringMap_Free (BStringMap *o);
const char * BStringMap_Get (const BStringMap *o, const char *key);
int BStringMap_Set (BStringMap *o, const char *key, const char *value) WARN_UNUSED;
void BStringMap_Unset (BStringMap *o, const char *key);
const char * BStringMap_First (const BStringMap *o);
const char * BStringMap_Next (const BStringMap *o, const char *key);

#endif
