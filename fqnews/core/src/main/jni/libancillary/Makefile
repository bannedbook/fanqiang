###########################################################################
# libancillary - black magic on Unix domain sockets
# (C) Nicolas George
# Makefile - guess what
###########################################################################

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. The name of the author may not be used to endorse or promote products
#     derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

CC=gcc
CFLAGS=-Wall -g -O0
LDFLAGS= -g
LIBS=
AR=ar
RANLIB=ranlib
RM=rm
CP=cp
MKDIR=mkdir
TAR=tar
GZIP=gzip -9

NAME=libancillary
DISTRIBUTION=API COPYING Makefile ancillary.h fd_send.c fd_recv.c test.c
VERSION=0.9.1

OBJECTS=fd_send.o fd_recv.o

TUNE_OPTS=#-DNDEBUG
#TUNE_OPTS=-DNDEBUG
#TUNE_OPTS=-DNDEBUG \
	-DSPARE_SEND_FDS -DSPARE_SEND_FD -DSPARE_RECV_FDS -DSPARE_RECV_FD

.c.o:
	$(CC) -c $(CFLAGS) $(TUNE_OPTS) $<

all: libancillary.a test evserver evclient

libancillary.a: $(OBJECTS)
	$(AR) cr $@ $(OBJECTS)
	$(RANLIB) $@

fd_send.o: ancillary.h
fd_recv.o: ancillary.h

test: test.c libancillary.a
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS)  test.c libancillary.a $(LIBS)

clean:
	-$(RM) -f *.o *.a test  evserver evclient

dist:
	$(MKDIR) $(NAME)-$(VERSION)
	$(CP) $(DISTRIBUTION) $(NAME)-$(VERSION)
	$(TAR) -cf - $(NAME)-$(VERSION) | $(GZIP) > $(NAME)-$(VERSION).tar.gz
	$(RM) -rf $(NAME)-$(VERSION)

evclient: evclient.c libancillary.a
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) -L. evclient.c -lancillary $(LIBS)

evserver: evserver.c libancillary.a
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) -L. evserver.c -lancillary $(LIBS)
