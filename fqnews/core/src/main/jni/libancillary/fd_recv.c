/***************************************************************************
 * libancillary - black magic on Unix domain sockets
 * (C) Nicolas George
 * fd_send.c - receiving file descriptors
 ***************************************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _XPG4_2 /* Solaris sucks */
# define _XPG4_2
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <assert.h>
#if defined(__FreeBSD__)
# include <sys/param.h> /* FreeBSD sucks */
#endif

#include "ancillary.h"

int
ancil_recv_fds_with_buffer(int sock, int *fds, unsigned n_fds, void *buffer)
{
    struct msghdr msghdr;
    char nothing;
    struct iovec nothing_ptr;
    struct cmsghdr *cmsg;
    int i;

    nothing_ptr.iov_base = &nothing;
    nothing_ptr.iov_len = 1;
    msghdr.msg_name = NULL;
    msghdr.msg_namelen = 0;
    msghdr.msg_iov = &nothing_ptr;
    msghdr.msg_iovlen = 1;
    msghdr.msg_flags = 0;
    msghdr.msg_control = buffer;
    msghdr.msg_controllen = sizeof(struct cmsghdr) + sizeof(int) * n_fds;
    cmsg = CMSG_FIRSTHDR(&msghdr);
    cmsg->cmsg_len = msghdr.msg_controllen;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    for(i = 0; i < n_fds; i++)
	((int *)CMSG_DATA(cmsg))[i] = -1;
    
    if(recvmsg(sock, &msghdr, 0) < 0)
	return(-1);
    for(i = 0; i < n_fds; i++)
	fds[i] = ((int *)CMSG_DATA(cmsg))[i];
    n_fds = (cmsg->cmsg_len - sizeof(struct cmsghdr)) / sizeof(int);
    return(n_fds);
}

#ifndef SPARE_RECV_FDS
int
ancil_recv_fds(int sock, int *fd, unsigned n_fds)
{
    ANCIL_FD_BUFFER(ANCIL_MAX_N_FDS) buffer;

    assert(n_fds <= ANCIL_MAX_N_FDS);
    return(ancil_recv_fds_with_buffer(sock, fd, n_fds, &buffer));
}
#endif /* SPARE_RECV_FDS */

#ifndef SPARE_RECV_FD
int
ancil_recv_fd(int sock, int *fd)
{
    ANCIL_FD_BUFFER(1) buffer;

    return(ancil_recv_fds_with_buffer(sock, fd, 1, &buffer) == 1 ? 0 : -1);
}
#endif /* SPARE_RECV_FD */
