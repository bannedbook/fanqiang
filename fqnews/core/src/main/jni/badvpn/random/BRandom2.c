/**
 * @file BRandom2.c
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "BRandom2.h"

static int do_init (BRandom2 *o)
{
    if (o->initialized) {
        return 1;
    }
    
    o->urandom_fd = open("/dev/urandom", O_RDONLY);
    if (o->urandom_fd < 0) {
        return 0;
    }
    
    o->initialized = 1;
    
    return 1;
}

int BRandom2_Init (BRandom2 *o, int flags)
{
    ASSERT((flags & ~(BRANDOM2_INIT_LAZY)) == 0)
    
    o->initialized = 0;
    
    if (!(flags & BRANDOM2_INIT_LAZY) && !do_init(o)) {
        return 0;
    }
    
    DebugObject_Init(&o->d_obj);
    return 1;
}

void BRandom2_Free (BRandom2 *o)
{
    DebugObject_Free(&o->d_obj);
    
    if (o->initialized) {
        close(o->urandom_fd);
    }
}

int BRandom2_GenBytes (BRandom2 *o, void *out, size_t len)
{
    DebugObject_Access(&o->d_obj);
    
    if (!do_init(o)) {
        return 0;
    }
    
    ssize_t res = read(o->urandom_fd, out, len);
    if (res < 0 || res != len) {
        return 0;
    }
    
    return 1;
}
