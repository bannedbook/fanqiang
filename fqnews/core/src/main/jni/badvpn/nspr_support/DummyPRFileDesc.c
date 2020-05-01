/**
 * @file DummyPRFileDesc.c
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
#include <string.h>
#include <limits.h>

#include <prerror.h>
#include <prmem.h>

#include <misc/debug.h>
#include <misc/offset.h>

#include <nspr_support/DummyPRFileDesc.h>

#ifndef NDEBUG
int dummyprfiledesc_initialized = 0;
#endif
PRDescIdentity dummyprfiledesc_identity;

static PRStatus method_close (PRFileDesc *fd)
{
    return PR_SUCCESS;
}

static PRStatus method_getpeername (PRFileDesc *fd, PRNetAddr *addr)
{
    PR_SetError(PR_UNKNOWN_ERROR, 0);
    return PR_FAILURE;
}

static PRIntn _PR_InvalidIntn (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRInt16 _PR_InvalidInt16 (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRInt32 _PR_InvalidInt32 (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRInt64 _PR_InvalidInt64 (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PROffset32 _PR_InvalidOffset32 (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PROffset64 _PR_InvalidOffset64 (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRStatus _PR_InvalidStatus (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return PR_FAILURE;
}

static PRFileDesc *_PR_InvalidDesc (void)
{
    ASSERT(0)
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return NULL;
}

static PRIOMethods methods = {
    (PRDescType)0,
    method_close,
    (PRReadFN)_PR_InvalidInt32,
    (PRWriteFN)_PR_InvalidInt32,
    (PRAvailableFN)_PR_InvalidInt32,
    (PRAvailable64FN)_PR_InvalidInt64,
    (PRFsyncFN)_PR_InvalidStatus,
    (PRSeekFN)_PR_InvalidOffset32,
    (PRSeek64FN)_PR_InvalidOffset64,
    (PRFileInfoFN)_PR_InvalidStatus,
    (PRFileInfo64FN)_PR_InvalidStatus,
    (PRWritevFN)_PR_InvalidInt32,
    (PRConnectFN)_PR_InvalidStatus,
    (PRAcceptFN)_PR_InvalidDesc,
    (PRBindFN)_PR_InvalidStatus,
    (PRListenFN)_PR_InvalidStatus,
    (PRShutdownFN)_PR_InvalidStatus,
    (PRRecvFN)_PR_InvalidInt32,
    (PRSendFN)_PR_InvalidInt32,
    (PRRecvfromFN)_PR_InvalidInt32,
    (PRSendtoFN)_PR_InvalidInt32,
    (PRPollFN)_PR_InvalidInt16,
    (PRAcceptreadFN)_PR_InvalidInt32,
    (PRTransmitfileFN)_PR_InvalidInt32,
    (PRGetsocknameFN)_PR_InvalidStatus,
    method_getpeername,
    (PRReservedFN)_PR_InvalidIntn,
    (PRReservedFN)_PR_InvalidIntn,
    (PRGetsocketoptionFN)_PR_InvalidStatus,
    (PRSetsocketoptionFN)_PR_InvalidStatus,
    (PRSendfileFN)_PR_InvalidInt32,
    (PRConnectcontinueFN)_PR_InvalidStatus,
    (PRReservedFN)_PR_InvalidIntn,
    (PRReservedFN)_PR_InvalidIntn,
    (PRReservedFN)_PR_InvalidIntn,
    (PRReservedFN)_PR_InvalidIntn
};

int DummyPRFileDesc_GlobalInit (void)
{
    ASSERT(!dummyprfiledesc_initialized)
    
    if ((dummyprfiledesc_identity = PR_GetUniqueIdentity("DummyPRFileDesc")) == PR_INVALID_IO_LAYER) {
        return 0;
    }
    
    #ifndef NDEBUG
    dummyprfiledesc_initialized = 1;
    #endif
    
    return 1;
}

void DummyPRFileDesc_Create (PRFileDesc *prfd)
{
    ASSERT(dummyprfiledesc_initialized)
    
    memset(prfd, 0, sizeof(*prfd));
    prfd->methods = &methods;
    prfd->secret = NULL;
    prfd->identity = dummyprfiledesc_identity;
}
