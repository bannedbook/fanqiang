#
# Copyright (c) 2001, 2002 Swedish Institute of Computer Science.
# All rights reserved. 
# 
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
# SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
# OF SUCH DAMAGE.
#
# This file is part of the lwIP TCP/IP stack.
# 
# Author: Adam Dunkels <adam@sics.se>
#

TESTDIR=$(LWIPDIR)/../test/unit
TESTFILES=$(TESTDIR)/lwip_unittests.c \
	$(TESTDIR)/api/test_sockets.c \
	$(TESTDIR)/arch/sys_arch.c \
	$(TESTDIR)/core/test_mem.c \
	$(TESTDIR)/core/test_pbuf.c \
	$(TESTDIR)/dhcp/test_dhcp.c \
	$(TESTDIR)/etharp/test_etharp.c \
	$(TESTDIR)/ip4/test_ip4.c \
	$(TESTDIR)/mdns/test_mdns.c \
	$(TESTDIR)/mqtt/test_mqtt.c \
	$(TESTDIR)/tcp/tcp_helper.c \
	$(TESTDIR)/tcp/test_tcp_oos.c \
	$(TESTDIR)/tcp/test_tcp.c \
	$(TESTDIR)/udp/test_udp.c

